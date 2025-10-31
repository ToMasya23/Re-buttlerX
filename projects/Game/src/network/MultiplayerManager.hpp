# pragma once
#include <Siv3D.hpp>
#include "BattleMessages.hpp"

// マルチプレイヤー通信管理クラス
class MultiplayerManager
{
public:
	enum class Role
	{
		None,
		Host,      // ホスト（サーバー役）
		Client,    // クライアント
	};

	MultiplayerManager();

	// 接続管理
	bool startHost(uint16 port = 12345);
	bool connect(const s3d::IPv4Address& address, uint16 port = 12345, double timeoutSeconds = 5.0);
	void disconnect();
	bool isConnected() const;
	bool isConnectionAlive() const;
	Role getRole() const { return m_role; }

	// メッセージ送信
	template<typename T>
	void send(const T& message);

	// メッセージ受信（型を指定して受信）
	template<typename T>
	s3d::Optional<T> receive();

	// メッセージタイプだけを確認（受信せずに覗き見）
	s3d::Optional<MessageType> peekMessageType();

	// 更新（毎フレーム呼び出し）
	void update();

private:
	s3d::TCPServer m_server;
	s3d::TCPClient m_client;
	Role m_role = Role::None;
	s3d::Array<s3d::Blob> m_receiveQueue;
	s3d::Optional<s3d::TCPSessionID> m_sessionID;  // ホスト側のセッションID
	bool m_clientConnectSucceeded = false;  // クライアント側のconnect()が成功したか
	
	// 接続監視用
	double m_connectionTimestamp = 0.0;
	double m_lastHeartbeatTime = 0.0;
	static constexpr double HEARTBEAT_INTERVAL = 5.0;  // 5秒ごと
	static constexpr double CONNECTION_TIMEOUT = 15.0;  // 15秒無応答で切断

	void processIncomingData();
	void checkConnectionHealth();
};
