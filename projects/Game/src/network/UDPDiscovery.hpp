#pragma once
#include <Siv3D.hpp>
#include "NetworkPlatform.hpp"
#include "HostDiscovery.hpp"  // HostInfo定義を使用

// UDPブロードキャストによるホスト検出
class UDPDiscovery
{
public:
	static constexpr uint16 DISCOVERY_PORT = 12344;  // ディスカバリ用ポート
	static constexpr uint16 GAME_PORT = 12345;       // ゲーム用TCPポート
	
	enum class Role { None, Host, Client };

	UDPDiscovery();
	~UDPDiscovery();

	// ホスト機能（合言葉で部屋を立てる）
	bool startAdvertising(const String& passphrase, const String& gameName, uint16 gamePort = GAME_PORT);
	void stopAdvertising();

	// クライアント機能（合言葉でブロードキャスト）
	bool startSearching(const String& passphrase);
	void stopSearching();
	const Array<HostInfo>& getDiscoveredHosts() const { return m_discoveredHosts; }

	// 共通
	void update();  // 毎フレーム呼び出し
	Role getRole() const { return m_role; }
	bool isActive() const { return m_role != Role::None; }

private:
	Role m_role = Role::None;
	NetworkPlatform::SocketHandle m_socket = NetworkPlatform::INVALID_SOCKET_HANDLE;

	// ホスト側の状態
	String m_passphrase;
	String m_gameName;
	uint16 m_gamePort = GAME_PORT;

	// クライアント側の状態
	Array<HostInfo> m_discoveredHosts;
	double m_lastBroadcastTime = 0.0;
	double m_searchStartTime = 0.0;
	static constexpr double BROADCAST_INTERVAL = 2.0;  // 2秒ごとにブロードキャスト
	static constexpr double SEARCH_TIMEOUT = 30.0;     // 30秒でタイムアウト

	// 内部処理
	void updateHost();
	void updateClient();
	bool isValidRequest(const Array<uint8>& data) const;
	bool isValidResponse(const Array<uint8>& data) const;
	void sendResponse(const IPv4Address& targetAddress, uint16 targetPort);
	void sendDiscoveryRequest();
	void addDiscoveredHost(const Array<uint8>& data, const IPv4Address& address, uint16 port);
};

// ディスカバリメッセージ構造体
#pragma pack(push, 1)

struct DiscoveryRequest
{
	uint32 magic = 0x52425458;  // "RBTX"
	uint8 version = 1;
	uint8 messageType = 0x01;   // REQUEST
	uint16 reserved = 0;
	char passphrase[64]{};      // 合言葉
};

struct DiscoveryResponse
{
	uint32 magic = 0x52425458;  // "RBTX"
	uint8 version = 1;
	uint8 messageType = 0x02;   // RESPONSE
	uint16 gamePort = 0;
	char gameName[32]{};
	char hostName[32]{};
	uint32 timestamp = 0;
	char passphraseHash[32]{};  // 将来の拡張用
};

#pragma pack(pop)
