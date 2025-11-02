#pragma once
#include <Siv3D.hpp>
#include <cstdint>
#include <type_traits>
#include <cstring>
#include "BattleMessages.hpp"

class MultiplayerManager
{
public:
	enum class Role
	{
		None,
		Host,
		Client,
	};

	enum class ConnectionState
	{
		Disconnected,
		Handshaking,
		Connected,
	};

	MultiplayerManager();

	bool startHost(uint16 port = 12345);
	bool connect(const s3d::IPv4Address& address, uint16 port = 12345, double timeoutSeconds = 5.0);
	void disconnect();

	bool isConnected() const;
	bool isConnectionAlive() const;
	Role getRole() const { return m_role; }
	ConnectionState getConnectionState() const { return m_connectionState; }

	template <typename T>
	bool send(const T& message);

	template <typename T>
	s3d::Optional<T> receive();

	net::PacketType peekPacketType() const;
	void discardFrontPacket();

	void update();

private:
	struct QueuedPacket
	{
		net::PacketHeader header;
		s3d::Array<uint8> payload;
	};

	s3d::TCPServer m_server;
	s3d::TCPClient m_client;
	Role m_role = Role::None;
	ConnectionState m_connectionState = ConnectionState::Disconnected;
	s3d::Array<QueuedPacket> m_receiveQueue;
	s3d::Optional<s3d::TCPSessionID> m_sessionID;
	bool m_clientConnectSucceeded = false;

	double m_connectionTimestamp = 0.0;
	double m_lastHeartbeatTime = 0.0;
	double m_lastSentHeartbeatTime = 0.0;

	uint32 m_nextSendSequence = 1;
	uint32 m_lastReceivedSequence = 0;

	uint32 m_localNonce = 0;
	uint32 m_remoteNonce = 0;
	bool m_localHandshakeSent = false;
	bool m_remoteHandshakeReceived = false;

	static constexpr size_t MaxReceiveQueue = 128;
	static constexpr double HEARTBEAT_INTERVAL = 5.0;
	static constexpr double CONNECTION_TIMEOUT = 15.0;

	void processIncomingData();
	void checkConnectionHealth();
	void sendHeartbeat();
	void sendHandshake();
	bool sendPacket(net::PacketType type, const void* payload, uint32 payloadSize, bool allowDuringHandshake = false);
	void enqueuePacket(const net::PacketHeader& header, s3d::Array<uint8>&& payload);
	void handleHandshakePacket(const net::PacketHeader& header, const s3d::Array<uint8>& payload);
	void resetState();
};

namespace net_detail
{
	template <typename T>
	struct MultiplayerMessageTraits;

	template <>
	struct MultiplayerMessageTraits<net::ActionRequestMessage>
	{
		static constexpr net::PacketType Type = net::PacketType::ActionRequest;
		static constexpr bool AllowDuringHandshake = false;
		static constexpr MultiplayerManager::Role SenderRole = MultiplayerManager::Role::Client;
	};

	template <>
	struct MultiplayerMessageTraits<net::StateSnapshotMessage>
	{
		static constexpr net::PacketType Type = net::PacketType::StateSnapshot;
		static constexpr bool AllowDuringHandshake = false;
		static constexpr MultiplayerManager::Role SenderRole = MultiplayerManager::Role::Host;
	};

	template <>
	struct MultiplayerMessageTraits<net::BattleEventMessage>
	{
		static constexpr net::PacketType Type = net::PacketType::BattleEvent;
		static constexpr bool AllowDuringHandshake = false;
		static constexpr MultiplayerManager::Role SenderRole = MultiplayerManager::Role::Host;
	};

	template <>
	struct MultiplayerMessageTraits<net::BattleEndMessage>
	{
		static constexpr net::PacketType Type = net::PacketType::BattleEnd;
		static constexpr bool AllowDuringHandshake = false;
		static constexpr MultiplayerManager::Role SenderRole = MultiplayerManager::Role::Host;
	};
}

template <typename T>
bool MultiplayerManager::send(const T& message)
{
	using Traits = net_detail::MultiplayerMessageTraits<T>;
	static_assert(std::is_trivially_copyable_v<T>, "Network messages must be trivially copyable");
	static_assert(sizeof(T) <= net::MaxPayloadSize, "Message exceeds maximum payload size");
	if (m_role != Traits::SenderRole)
	{
		//Console << U"[MultiplayerManager] Attempted to send message from invalid role";
		return false;
	}
	return sendPacket(Traits::Type, &message, static_cast<uint32>(sizeof(T)), Traits::AllowDuringHandshake);
}

template <typename T>
s3d::Optional<T> MultiplayerManager::receive()
{
	using Traits = net_detail::MultiplayerMessageTraits<T>;
	if (m_receiveQueue.isEmpty())
	{
		return s3d::none;
	}

	const auto& packet = m_receiveQueue.front();
	if (packet.header.type != static_cast<uint16>(Traits::Type))
	{
		return s3d::none;
	}

	if (packet.payload.size() != sizeof(T))
	{
		//Console << U"[MultiplayerManager] Payload size mismatch";
		m_receiveQueue.pop_front();
		return s3d::none;
	}

	T message{};
	std::memcpy(&message, packet.payload.data(), sizeof(T));
	m_receiveQueue.pop_front();
	return message;
}
