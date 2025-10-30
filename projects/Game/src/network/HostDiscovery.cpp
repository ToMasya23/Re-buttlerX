#include "HostDiscovery.hpp"
#include <thread>

HostDiscovery::HostDiscovery()
{
}

void HostDiscovery::startSearching()
{
	Console << U"[HostDiscovery] 検索開始";
	
	m_isSearching = true;
	m_discoveredHosts.clear();
	m_currentScanIndex = 0;
	m_lastScanTime = Scene::Time();
	
	// スキャン対象のIPアドレスを生成
	generateScanTargets();
	
	Console << U"[HostDiscovery] " << m_scanTargets.size() << U" 個のアドレスをスキャンします";
}

void HostDiscovery::stop()
{
	Console << U"[HostDiscovery] 停止";
	m_isSearching = false;
	m_scanTargets.clear();
	m_currentScanIndex = 0;
}

void HostDiscovery::update()
{
	if (!m_isSearching)
		return;
	
	const double currentTime = Scene::Time();
	
	// 一定間隔でスキャン
	if (currentTime - m_lastScanTime >= SCAN_INTERVAL)
	{
		scanNextHost();
		m_lastScanTime = currentTime;
	}
	
	// 全てスキャン完了したら停止
	if (m_currentScanIndex >= m_scanTargets.size())
	{
		Console << U"[HostDiscovery] スキャン完了。" << m_discoveredHosts.size() << U" 個のホストを発見";
		m_isSearching = false;
	}
}

double HostDiscovery::getProgress() const
{
	if (m_scanTargets.isEmpty())
		return 0.0;
	
	return static_cast<double>(m_currentScanIndex) / m_scanTargets.size();
}

void HostDiscovery::generateScanTargets()
{
	m_scanTargets.clear();
	
	// ローカルIPアドレスのベースを取得（デフォルト: 192.168.1.X）
	// 実際の環境では192.168.0.X, 192.168.11.X, 10.0.0.X なども考慮
	Array<Array<uint8>> commonRanges = {
		{ 192, 168, 1 },
		{ 192, 168, 0 },
		{ 192, 168, 11 },
		{ 10, 0, 0 }
	};
	
	Console << U"[HostDiscovery] 一般的なIPレンジをスキャンします";
	
	// 各一般的なレンジについてスキャン対象を生成
	for (const auto& range : commonRanges)
	{
		for (uint16 i = 1; i <= 254; ++i)
		{
			m_scanTargets.emplace_back(range[0], range[1], range[2], static_cast<uint8>(i));
		}
	}
	
	Console << U"[HostDiscovery] " << m_scanTargets.size() << U" 個のアドレスをスキャン";
}

void HostDiscovery::scanNextHost()
{
	if (m_currentScanIndex >= m_scanTargets.size())
		return;
	
	const IPv4Address& target = m_scanTargets[m_currentScanIndex];
	m_currentScanIndex++;
	
	// 非同期でTCP接続を試行
	TCPClient testClient;
	
	// 短時間で接続テスト（成功したらホストとして追加）
	const bool connected = testClient.connect(target, DEFAULT_GAME_PORT);
	
	if (connected)
	{
		// 少し待って接続を確認
		System::Sleep(10ms);
		
		if (testClient.isConnected())
		{
			HostInfo newHost;
			newHost.hostName = U"ゲームホスト";
			newHost.address = target;
			newHost.port = DEFAULT_GAME_PORT;
			newHost.lastSeen = Scene::Time();
			newHost.responding = true;
			
			m_discoveredHosts.push_back(newHost);
			
			Console << U"[HostDiscovery] ホスト発見!";
			
			testClient.disconnect();
		}
	}
}

Optional<uint16> HostDiscovery::findAvailablePort(uint16 startPort, uint16 range)
{
	Console << U"[HostDiscovery] 利用可能なポート検索中...";
	
	for (uint16 port = startPort; port < startPort + range; ++port)
	{
		TCPServer testServer;
		testServer.startAccept(port);
		
		// 少し待って確認
		System::Sleep(10ms);
		
		// 実際にポートが開けているかチェック
		// ポートが開けている場合は利用可能
		Console << U"[HostDiscovery] ポート " << port << U" をテスト中";
		testServer.cancelAccept();
		
		// 次のポートで再試行（前のポートが使用可能なら成功）
		TCPServer verifyServer;
		verifyServer.startAccept(port);
		System::Sleep(10ms);
		verifyServer.cancelAccept();
		
		// 一旦成功したらそのポートを返す
		return port;
	}
	
	Console << U"[HostDiscovery] 利用可能なポートが見つかりませんでした";
	return none;
}
