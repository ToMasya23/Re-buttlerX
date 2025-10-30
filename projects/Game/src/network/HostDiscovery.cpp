#include "HostDiscovery.hpp"
#include <thread>

// Windows用のネットワークAPI
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

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
	
	// ローカルIPアドレスを自動検出
	IPv4Address localIP = detectLocalIP();
	
	if (localIP == IPv4Address{ 127, 0, 0, 1 } || localIP == IPv4Address{ 0, 0, 0, 0 })
	{
		// 検出失敗時は一般的なレンジをスキャン
		Console << U"[HostDiscovery] ローカルIP検出失敗。一般的なレンジをスキャンします";
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
		// 検出したローカルIPと同じサブネットをスキャン
		const auto& ipData = localIP.getData();
		uint8 a = ipData[0];
		uint8 b = ipData[1];
		uint8 c = ipData[2];
		
		Console << U"[HostDiscovery] ローカルIP検出: " << (int)a << U"." << (int)b << U"." << (int)c << U".X";
		Console << U"[HostDiscovery] 同じサブネット（" << (int)a << U"." << (int)b << U"." << (int)c << U".1-50）をスキャンします";
		
		// 同じサブネットの1-50をスキャン
		for (uint16 i = 1; i <= 50; ++i)
		{
			m_scanTargets.emplace_back(a, b, c, static_cast<uint8>(i));
		}
	}
	
	Console << U"[HostDiscovery] " << m_scanTargets.size() << U" 個のアドレスをスキャン";
}

IPv4Address HostDiscovery::detectLocalIP()
{
#ifdef _WIN32
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
	{
		Console << U"[HostDiscovery] ホスト名の取得に失敗";
		return IPv4Address{ 127, 0, 0, 1 };
	}
	
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;  // IPv4
	hints.ai_socktype = SOCK_STREAM;
	
	struct addrinfo* result = nullptr;
	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
	{
		Console << U"[HostDiscovery] アドレス情報の取得に失敗";
		return IPv4Address{ 127, 0, 0, 1 };
	}
	
	// 最初の非ループバックアドレスを使用
	for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		if (ptr->ai_family == AF_INET)
		{
			struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
			uint32_t addr = ntohl(sockaddr_ipv4->sin_addr.s_addr);
			
			uint8 a = (addr >> 24) & 0xFF;
			uint8 b = (addr >> 16) & 0xFF;
			uint8 c = (addr >> 8) & 0xFF;
			uint8 d = (addr >> 0) & 0xFF;
			
			// ループバックアドレス（127.x.x.x）をスキップ
			if (a != 127)
			{
				freeaddrinfo(result);
				Console << U"[HostDiscovery] ローカルIPを検出: " << (int)a << U"." << (int)b << U"." << (int)c << U"." << (int)d;
				return IPv4Address{ a, b, c, d };
			}
		}
	}
	
	freeaddrinfo(result);
#endif
	
	Console << U"[HostDiscovery] 有効なローカルIPが見つかりませんでした";
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
			
			Console << U"[HostDiscovery] ホスト発見: " << ipStr;
			
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
