#include "UDPDiscovery.hpp"
#include <cstring>

UDPDiscovery::UDPDiscovery()
{
	NetworkPlatform::InitializeNetworking();
}

UDPDiscovery::~UDPDiscovery()
{
	stopAdvertising();
	stopSearching();
	NetworkPlatform::ShutdownNetworking();
}

bool UDPDiscovery::startAdvertising(const String& passphrase, const String& gameName, uint16 gamePort)
{
	if (m_role != Role::None)
	{
		Console << U"[UDPDiscovery] Already active in another role";
		return false;
	}

	Console << U"[UDPDiscovery] Starting host with passphrase: " << passphrase;

	m_passphrase = passphrase;
	m_gameName = gameName;
	m_gamePort = gamePort;

	// UDPソケット作成
	m_socket = NetworkPlatform::CreateUDPSocket();
	if (m_socket == NetworkPlatform::INVALID_SOCKET_HANDLE)
	{
		Console << U"[UDPDiscovery] Failed to create UDP socket";
		return false;
	}

	// 非ブロッキングモード
	if (!NetworkPlatform::SetNonBlocking(m_socket, true))
	{
		NetworkPlatform::CloseSocket(m_socket);
		m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;
		return false;
	}

	// SO_REUSEADDR設定
	NetworkPlatform::SetReuseAddress(m_socket, true);

	// ポートにバインド
	if (!NetworkPlatform::BindSocket(m_socket, DISCOVERY_PORT))
	{
		Console << U"[UDPDiscovery] Failed to bind to port " << DISCOVERY_PORT;
		NetworkPlatform::CloseSocket(m_socket);
		m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;
		return false;
	}

	m_role = Role::Host;
	Console << U"[UDPDiscovery] Host started successfully on port " << DISCOVERY_PORT;
	Console << U"[演出] SNSに投稿しました！合言葉: " << passphrase;
	return true;
}

void UDPDiscovery::stopAdvertising()
{
	if (m_role == Role::Host)
	{
		Console << U"[UDPDiscovery] Stopping host";
		NetworkPlatform::CloseSocket(m_socket);
		m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;
		m_role = Role::None;
	}
}

bool UDPDiscovery::startSearching(const String& passphrase)
{
	if (m_role != Role::None)
	{
		Console << U"[UDPDiscovery] Already active in another role";
		return false;
	}

	Console << U"[UDPDiscovery] Starting client search with passphrase: " << passphrase;

	m_passphrase = passphrase;
	m_discoveredHosts.clear();
	m_searchStartTime = Scene::Time();
	m_lastBroadcastTime = 0.0;

	// UDPソケット作成
	m_socket = NetworkPlatform::CreateUDPSocket();
	if (m_socket == NetworkPlatform::INVALID_SOCKET_HANDLE)
	{
		Console << U"[UDPDiscovery] Failed to create UDP socket";
		return false;
	}

	// 非ブロッキングモード
	if (!NetworkPlatform::SetNonBlocking(m_socket, true))
	{
		NetworkPlatform::CloseSocket(m_socket);
		m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;
		return false;
	}

	// ブロードキャスト許可
	if (!NetworkPlatform::SetBroadcast(m_socket, true))
	{
		Console << U"[UDPDiscovery] Failed to enable broadcast";
		NetworkPlatform::CloseSocket(m_socket);
		m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;
		return false;
	}

	m_role = Role::Client;
	Console << U"[UDPDiscovery] Client search started";
	Console << U"[演出] 噛みつき中...";
	return true;
}

void UDPDiscovery::stopSearching()
{
	if (m_role == Role::Client)
	{
		Console << U"[UDPDiscovery] Stopping client search";
		NetworkPlatform::CloseSocket(m_socket);
		m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;
		m_role = Role::None;
	}
}

void UDPDiscovery::update()
{
	if (m_role == Role::Host)
	{
		updateHost();
	}
	else if (m_role == Role::Client)
	{
		updateClient();
	}
}

void UDPDiscovery::updateHost()
{
	// リクエストを受信
	uint8 buffer[1024];
	auto recvResult = NetworkPlatform::RecvFrom(m_socket, buffer, sizeof(buffer));

	if (recvResult.bytesReceived > 0)
	{
		Array<uint8> data(buffer, buffer + recvResult.bytesReceived);

		if (isValidRequest(data))
		{
			DiscoveryRequest* req = reinterpret_cast<DiscoveryRequest*>(data.data());
			String receivedPassphrase = Unicode::FromUTF8(req->passphrase);

			// 合言葉チェック
			if (receivedPassphrase == m_passphrase)
			{
				Console << U"[Host] ゲストが噛みついてきた！合言葉一致: " << receivedPassphrase;
				Console << U"[演出] 噛みついた相手を発見！";

				// レスポンス送信
				sendResponse(recvResult.senderAddress, recvResult.senderPort);
			}
			else
			{
				Console << U"[Host] 合言葉不一致。無視します。(受信: " << receivedPassphrase << U")";
			}
		}
	}
}

void UDPDiscovery::updateClient()
{
	const double currentTime = Scene::Time();

	// 一定間隔でブロードキャスト送信
	if (currentTime - m_lastBroadcastTime >= BROADCAST_INTERVAL)
	{
		Console << U"[Client] 合言葉「" << m_passphrase << U"」で噛みつき中...";
		sendDiscoveryRequest();
		m_lastBroadcastTime = currentTime;
	}

	// レスポンスを受信
	uint8 buffer[1024];
	auto recvResult = NetworkPlatform::RecvFrom(m_socket, buffer, sizeof(buffer));

	if (recvResult.bytesReceived > 0)
	{
		Array<uint8> data(buffer, buffer + recvResult.bytesReceived);

		if (isValidResponse(data))
		{
			Console << U"[Client] マッチング成功！ホストを発見: " << recvResult.senderAddress.str();
			Console << U"[演出] 噛みついた相手を発見！接続します...";
			addDiscoveredHost(data, recvResult.senderAddress, recvResult.senderPort);
		}
	}

	// タイムアウトチェック
	if (currentTime - m_searchStartTime > SEARCH_TIMEOUT)
	{
		Console << U"[Client] タイムアウト: ホストが見つかりませんでした（30秒経過）";
		stopSearching();
	}
}

bool UDPDiscovery::isValidRequest(const Array<uint8>& data) const
{
	if (data.size() < sizeof(DiscoveryRequest))
		return false;

	const DiscoveryRequest* req = reinterpret_cast<const DiscoveryRequest*>(data.data());
	return req->magic == 0x52425458 && req->version == 1 && req->messageType == 0x01;
}

bool UDPDiscovery::isValidResponse(const Array<uint8>& data) const
{
	if (data.size() < sizeof(DiscoveryResponse))
		return false;

	const DiscoveryResponse* resp = reinterpret_cast<const DiscoveryResponse*>(data.data());
	return resp->magic == 0x52425458 && resp->version == 1 && resp->messageType == 0x02;
}

void UDPDiscovery::sendResponse(const IPv4Address& targetAddress, uint16 targetPort)
{
	DiscoveryResponse response{};
	response.gamePort = m_gamePort;

	// ゲーム名をUTF-8でコピー
	std::string gameNameUTF8 = m_gameName.toUTF8();
	strncpy_s(response.gameName, sizeof(response.gameName), gameNameUTF8.c_str(), _TRUNCATE);

	// ホスト名を取得
	String hostName = System::ComputerName();
	std::string hostNameUTF8 = hostName.toUTF8();
	strncpy_s(response.hostName, sizeof(response.hostName), hostNameUTF8.c_str(), _TRUNCATE);

	response.timestamp = static_cast<uint32>(Time::GetSecSinceEpoch());

	// 送信
	int32 sent = NetworkPlatform::SendTo(
		m_socket,
		&response,
		sizeof(response),
		targetAddress,
		targetPort
	);

	if (sent > 0)
	{
		Console << U"[UDPDiscovery] Response sent to " << targetAddress.str() << U":" << targetPort;
	}
}

void UDPDiscovery::sendDiscoveryRequest()
{
	DiscoveryRequest request{};

	// 合言葉をUTF-8でコピー
	std::string passphraseUTF8 = m_passphrase.toUTF8();
	strncpy_s(request.passphrase, sizeof(request.passphrase), passphraseUTF8.c_str(), _TRUNCATE);

	// ブロードキャストアドレスに送信
	IPv4Address broadcastAddr{ 255, 255, 255, 255 };
	int32 sent = NetworkPlatform::SendTo(
		m_socket,
		&request,
		sizeof(request),
		broadcastAddr,
		DISCOVERY_PORT
	);

	if (sent > 0)
	{
		Console << U"[UDPDiscovery] Broadcast request sent";
	}
}

void UDPDiscovery::addDiscoveredHost(const Array<uint8>& data, const IPv4Address& address, uint16 port)
{
	(void)port;  // 未使用パラメータ（将来の拡張用に保持）
	
	const DiscoveryResponse* resp = reinterpret_cast<const DiscoveryResponse*>(data.data());

	HostInfo info{};
	info.hostName = Unicode::FromUTF8(resp->hostName);
	info.address = address;
	info.port = resp->gamePort;
	info.lastSeen = Scene::Time();
	info.responding = true;

	// 既存ホストの更新または新規追加
	bool found = false;
	for (auto& host : m_discoveredHosts)
	{
		if (host.address == address)
		{
			host = info;
			found = true;
			break;
		}
	}

	if (!found)
	{
		m_discoveredHosts.push_back(info);
		Console << U"[UDPDiscovery] New host discovered: " << info.hostName << U" (" << address.str() << U":" << info.port << U")";
	}
}
