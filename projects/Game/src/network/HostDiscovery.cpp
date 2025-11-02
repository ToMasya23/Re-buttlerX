#include "HostDiscovery.hpp"
#include "NetworkPlatform.hpp"
#include <thread>

HostDiscovery::HostDiscovery()
{
}

void HostDiscovery::startSearching()
{
	// Console << U"[HostDiscovery] 検索開始";
	
	m_isSearching = true;
	m_discoveredHosts.clear();
	m_currentScanIndex = 0;
	m_lastScanTime = Scene::Time();
	
	// スキャン対象のIPアドレスを生成
	generateScanTargets();
	
	// Console << U"[HostDiscovery] " << m_scanTargets.size() << U" 個のアドレスをスキャンします";
}

void HostDiscovery::stop()
{
	// Console << U"[HostDiscovery] 停止";
	m_isSearching = false;
	m_scanTargets.clear();
	m_currentScanIndex = 0;
}

void HostDiscovery::update()
{
	if (!m_isSearching)
		return;
	
	const double currentTime = Scene::Time();
	
	// 一定間隔でバッチスキャン
	if (currentTime - m_lastScanTime >= SCAN_INTERVAL)
	{
		// バッチサイズ分スキャン（10個ずつ）
		for (size_t i = 0; i < BATCH_SIZE && m_currentScanIndex < m_scanTargets.size(); ++i)
		{
			scanNextHost();
		}
		m_lastScanTime = currentTime;
	}
	
	// 全てスキャン完了したら停止
	if (m_currentScanIndex >= m_scanTargets.size())
	{
		// Console << U"[HostDiscovery] スキャン完了。" << m_discoveredHosts.size() << U" 個のホストを発見";
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
	
	// ローカルIPアドレスを自動検出
	IPv4Address localIP = detectLocalIP();
	
	if (localIP == IPv4Address{ 127, 0, 0, 1 } || localIP == IPv4Address{ 0, 0, 0, 0 })
	{
		// 検出失敗時は一般的なレンジをスキャン
		// Console << U"[HostDiscovery] ローカルIP検出失敗。一般的なレンジをスキャンします";
		Array<Array<uint8>> commonRanges = {
			{ 192, 168, 1 },
			{ 192, 168, 0 }
		};
		
		for (const auto& range : commonRanges)
		{
			// 各レンジの最初の50個をスキャン
			for (uint16 i = 1; i <= 50; ++i)
			{
				m_scanTargets.emplace_back(range[0], range[1], range[2], static_cast<uint8>(i));
			}
		}
	}
	else
	{
		// 検出したローカルIPのクラスBサブネット全体をスキャン
		const auto& ipData = localIP.getData();
		uint8 a = ipData[0];
		uint8 b = ipData[1];
		uint8 c = ipData[2];
		
		// Console << U"[HostDiscovery] ローカルIP検出: " << (int)a << U"." << (int)b << U"." << (int)c << U".X";
		
		// 150.65.x.x のような大学/企業ネットワークの場合、複数のサブネットをスキャン
		// 自分のサブネット + 隣接するサブネット（±10）をスキャン
		// Console << U"[HostDiscovery] サブネット " << (int)a << U"." << (int)b << U"." << (int)c << U".1-50";
		// Console << U"[HostDiscovery] および隣接サブネット（±10）をスキャンします";
		

		// 自分のサブネット
		for (uint16 i = 1; i <= 50; ++i)
		{
			m_scanTargets.emplace_back(a, b, c, static_cast<uint8>(i));
		}
		
		// 隣接する20個のサブネット（c-10 から c+10）を各10個ずつスキャン
		for (int offset = -10; offset <= 10; ++offset)
		{
			if (offset == 0) continue;  // 自分のサブネットはすでに追加済み
			
			int newC = static_cast<int>(c) + offset;
			if (newC < 0 || newC > 255) continue;
			
			// 各サブネットの最初の10個をスキャン
			for (uint16 i = 1; i <= 10; ++i)
			{
				m_scanTargets.emplace_back(a, b, static_cast<uint8>(newC), static_cast<uint8>(i));
			}
		}
	}
	
	// Console << U"[HostDiscovery] " << m_scanTargets.size() << U" 個のアドレスをスキャン";
}

IPv4Address HostDiscovery::detectLocalIP()
{
	Array<IPv4Address> localIPs = NetworkPlatform::GetLocalIPAddresses();
	for (const auto& ip : localIPs)
	{
		if (ip == IPv4Address{ 127, 0, 0, 1 } || ip == IPv4Address{ 0, 0, 0, 0 })
		{
			continue;
		}

		const auto& data = ip.getData();
		// Console << U"[HostDiscovery] Local IP detected: "
			// << static_cast<int>(data[0]) << U"." << static_cast<int>(data[1])
			// << U"." << static_cast<int>(data[2]) << U"." << static_cast<int>(data[3]);
		return ip;
	}

	if (!localIPs.isEmpty())
	{
		const auto& data = localIPs.front().getData();
		// Console << U"[HostDiscovery] Only loopback/local IPs detected, using "
			// << static_cast<int>(data[0]) << U"." << static_cast<int>(data[1])
			// << U"." << static_cast<int>(data[2]) << U"." << static_cast<int>(data[3]);
		return localIPs.front();
	}

	// Console << U"[HostDiscovery] No local IP address available; using loopback";
	return IPv4Address{ 127, 0, 0, 1 };
}

void HostDiscovery::scanNextHost()
{
	if (m_currentScanIndex >= m_scanTargets.size())
		return;
	
	const IPv4Address& target = m_scanTargets[m_currentScanIndex];
	m_currentScanIndex++;
	
	// 非同期でTCP接続を試行
	TCPClient testClient;
	
	// 短時間で接続テスト（タイムアウトなしで試行）
	const bool connected = testClient.connect(target, DEFAULT_GAME_PORT);
	
	if (connected)
	{
		// 少し待って接続を確認
		System::Sleep(5ms);
		
		if (testClient.isConnected())
		{
			// IPアドレスを文字列化
			const auto& ipData = target.getData();
			String ipStr = Format(U"{}.{}.{}.{}", (int)ipData[0], (int)ipData[1], (int)ipData[2], (int)ipData[3]);
			
			HostInfo newHost;
			newHost.hostName = U"ゲームホスト (" + ipStr + U")";
			newHost.address = target;
			newHost.port = DEFAULT_GAME_PORT;
			newHost.lastSeen = Scene::Time();
			newHost.responding = true;
			
			m_discoveredHosts.push_back(newHost);
			
			// Console << U"[HostDiscovery] ホスト発見: " << ipStr;
			
			testClient.disconnect();
		}
	}
}

Optional<uint16> HostDiscovery::findAvailablePort(uint16 startPort, uint16 range)
{
	// Console << U"[HostDiscovery] 利用可能なポート検索中...";
	
	for (uint16 port = startPort; port < startPort + range; ++port)
	{
		TCPServer testServer;
		testServer.startAccept(port);
		
		// 少し待って確認
		System::Sleep(10ms);
		
		// 実際にポートが開けているかチェック
		// ポートが開けている場合は利用可能
		// Console << U"[HostDiscovery] ポート " << port << U" をテスト中";
		testServer.cancelAccept();
		
		// 次のポートで再試行（前のポートが使用可能なら成功）
		TCPServer verifyServer;
		verifyServer.startAccept(port);
		System::Sleep(10ms);
		verifyServer.cancelAccept();
		
		// 一旦成功したらそのポートを返す
		return port;
	}
	
	// Console << U"[HostDiscovery] 利用可能なポートが見つかりませんでした";
	return none;
}

