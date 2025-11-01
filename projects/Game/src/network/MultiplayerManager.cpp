#include "MultiplayerManager.hpp"
#include <limits>
#include <utility>

namespace
{
	uint32 GenerateNonce()
	{
		return s3d::Random(static_cast<uint32>(1), std::numeric_limits<uint32>::max());
	}

	net::PacketType ToPacketType(uint16 raw)
	{
		return static_cast<net::PacketType>(raw);
	}
}

MultiplayerManager::MultiplayerManager()
{
	resetState();
}

bool MultiplayerManager::startHost(uint16 port)
{
	Console << U"[MultiplayerManager] Starting host on port " << port;

	m_client.disconnect();
	m_server.cancelAccept();
	resetState();

	m_role = Role::Host;
	m_server.startAccept(port);

	Console << U"[Host] Waiting for client connection. Share your IP address with your opponent.";
	return true;
}

bool MultiplayerManager::connect(const s3d::IPv4Address& address, uint16 port, double timeoutSeconds)
{
	Console << U"[MultiplayerManager] Connecting to " << address.str() << U":" << port;

	m_client.disconnect();
	m_server.cancelAccept();
	resetState();

	if (!m_client.connect(address, port))
	{
		Console << U"[MultiplayerManager] connect() failed";
		m_role = Role::None;
		return false;
	}

	m_role = Role::Client;
	m_connectionState = ConnectionState::Handshaking;

	double timeout = Max(0.0, timeoutSeconds);
	const double startTime = Scene::Time();
	bool connected = m_client.isConnected();

	while (!connected && (Scene::Time() - startTime) <= timeout)
	{
		System::Sleep(50ms);
		connected = m_client.isConnected();
	}

	const double now = Scene::Time();
	m_clientConnectSucceeded = connected;
	m_connectionTimestamp = now;
	m_lastHeartbeatTime = now;
	m_lastSentHeartbeatTime = now;

	sendHandshake();
	return true;
}

void MultiplayerManager::disconnect()
{
	m_client.disconnect();
	m_server.cancelAccept();
	resetState();
	m_role = Role::None;
}

bool MultiplayerManager::isConnected() const
{
	return (m_connectionState == ConnectionState::Connected);
}

bool MultiplayerManager::isConnectionAlive() const
{
	if (!isConnected())
	{
		return false;
	}

	const double elapsed = Scene::Time() - m_lastHeartbeatTime;
	return (elapsed <= CONNECTION_TIMEOUT);
}

void MultiplayerManager::update()
{
	if (m_role == Role::Host)
	{
		if (!m_sessionID)
		{
			if (m_server.hasSession())
			{
				auto sessions = m_server.getSessionIDs();
				if (!sessions.isEmpty())
				{
					resetState();
					m_sessionID = sessions.front();
					m_connectionState = ConnectionState::Handshaking;
					m_connectionTimestamp = Scene::Time();
					m_lastHeartbeatTime = m_connectionTimestamp;
					m_lastSentHeartbeatTime = m_connectionTimestamp;
					Console << U"[Host] Client session accepted";
					sendHandshake();
				}
			}
		}
		else if (m_connectionState == ConnectionState::Handshaking && !m_localHandshakeSent)
		{
			sendHandshake();
		}
	}
	else if (m_role == Role::Client)
	{
		if (m_connectionState == ConnectionState::Handshaking && !m_localHandshakeSent)
		{
			sendHandshake();
		}
	}

	processIncomingData();
	checkConnectionHealth();
}

void MultiplayerManager::processIncomingData()
{
	auto readPackets = [&](auto availableFn, auto lookaheadFn, auto readFn)
	{
		net::PacketHeader header;

		while (availableFn() >= static_cast<int64>(sizeof(net::PacketHeader)))
		{
			if (!lookaheadFn(header))
			{
				break;
			}

			if (header.magic != net::PacketMagic)
			{
				Console << U"[MultiplayerManager] Invalid packet magic";
				s3d::Array<uint8> junk(sizeof(net::PacketHeader));
				readFn(junk.data(), junk.size());
				continue;
			}

			if (header.version != net::ProtocolVersion)
			{
				Console << U"[MultiplayerManager] Protocol version mismatch";
				s3d::Array<uint8> junk(sizeof(net::PacketHeader));
				readFn(junk.data(), junk.size());
				disconnect();
				return;
			}

			if (header.payloadSize > net::MaxPayloadSize)
			{
				Console << U"[MultiplayerManager] Payload too large: " << header.payloadSize;
				s3d::Array<uint8> junk(sizeof(net::PacketHeader));
				readFn(junk.data(), junk.size());
				continue;
			}

			size_t totalSize = sizeof(net::PacketHeader) + header.payloadSize;
			if (availableFn() < static_cast<int64>(totalSize))
			{
				break;
			}

			if (!readFn(&header, sizeof(header)))
			{
				break;
			}

			s3d::Array<uint8> payload(header.payloadSize);
			if (header.payloadSize > 0)
			{
				if (!readFn(payload.data(), header.payloadSize))
				{
					break;
				}

				uint32 checksum = net::ComputeChecksum(payload.data(), header.payloadSize);
				if (checksum != header.checksum)
				{
					Console << U"[MultiplayerManager] Packet checksum mismatch";
					continue;
				}
			}

			m_lastHeartbeatTime = Scene::Time();

			net::PacketType type = ToPacketType(header.type);
			if (type == net::PacketType::Heartbeat)
			{
				continue;
			}

			if (type == net::PacketType::Handshake)
			{
				handleHandshakePacket(header, payload);
				continue;
			}

			if (header.sequence != 0 && header.sequence <= m_lastReceivedSequence)
			{
				Console << U"[MultiplayerManager] Dropping out-of-order packet seq=" << header.sequence;
				continue;
			}

			m_lastReceivedSequence = Max(m_lastReceivedSequence, header.sequence);

			if (m_connectionState != ConnectionState::Connected)
			{
				Console << U"[MultiplayerManager] Buffering skipped until handshake completes";
				continue;
			}

			enqueuePacket(header, std::move(payload));
		}
	};

	if (m_role == Role::Host)
	{
		if (!m_sessionID)
		{
			return;
		}

		auto availableFn = [&]() -> int64
		{
			return m_server.available(m_sessionID);
		};

		auto lookaheadFn = [&](net::PacketHeader& header) -> bool
		{
			return m_server.lookahead(header, m_sessionID);
		};

		auto readFn = [&](void* dst, size_t size) -> bool
		{
			return m_server.read(dst, size, m_sessionID);
		};

		readPackets(availableFn, lookaheadFn, readFn);
	}
	else if (m_role == Role::Client)
	{
		auto availableFn = [&]() -> int64
		{
			return m_client.available();
		};

		auto lookaheadFn = [&](net::PacketHeader& header) -> bool
		{
			return m_client.lookahead(header);
		};

		auto readFn = [&](void* dst, size_t size) -> bool
		{
			return m_client.read(dst, size);
		};

		readPackets(availableFn, lookaheadFn, readFn);
	}
}

void MultiplayerManager::handleHandshakePacket(const net::PacketHeader& header, const s3d::Array<uint8>& payload)
{
	if (payload.size() != sizeof(net::HandshakeMessage))
	{
		Console << U"[MultiplayerManager] Invalid handshake payload";
		disconnect();
		return;
	}

	net::HandshakeMessage handshake{};
	std::memcpy(&handshake, payload.data(), sizeof(handshake));

	m_remoteNonce = handshake.nonce;
	m_remoteHandshakeReceived = true;

	if (m_role == Role::Host && handshake.role != net::ConnectionRole::Client)
	{
		Console << U"[Host] Unexpected handshake role";
		return;
	}

	if (m_role == Role::Client && handshake.role != net::ConnectionRole::Host)
	{
		Console << U"[Client] Unexpected handshake role";
		return;
	}

	if (!m_localHandshakeSent)
	{
		sendHandshake();
	}

	if (m_connectionState != ConnectionState::Connected)
	{
		m_connectionState = ConnectionState::Connected;
		m_clientConnectSucceeded = true;
		m_connectionTimestamp = Scene::Time();
		m_lastHeartbeatTime = m_connectionTimestamp;
		m_lastSentHeartbeatTime = m_connectionTimestamp;
		Console << U"[MultiplayerManager] Handshake complete";
	}
}

void MultiplayerManager::enqueuePacket(const net::PacketHeader& header, s3d::Array<uint8>&& payload)
{
	if (m_receiveQueue.size() >= MaxReceiveQueue)
	{
		m_receiveQueue.pop_front();
	}

	m_receiveQueue.push_back(QueuedPacket{ header, std::move(payload) });
}

void MultiplayerManager::checkConnectionHealth()
{
	const double now = Scene::Time();

	if (m_role == Role::Host)
	{
		if (m_sessionID && !m_server.hasSession(*m_sessionID))
		{
			Console << U"[MultiplayerManager] Host session closed";
			resetState();
			return;
		}
	}
	else if (m_role == Role::Client)
	{
		if (!m_client.isConnected())
		{
			if (m_clientConnectSucceeded)
			{
				Console << U"[MultiplayerManager] Client connection lost";
			}
			m_clientConnectSucceeded = false;
		}
	}

	if (m_connectionState == ConnectionState::Disconnected)
	{
		return;
	}

	if ((now - m_lastHeartbeatTime) > CONNECTION_TIMEOUT)
	{
		Console << U"[MultiplayerManager] Connection timeout";
		disconnect();
		return;
	}

	if ((now - m_lastSentHeartbeatTime) >= HEARTBEAT_INTERVAL)
	{
		sendHeartbeat();
		m_lastSentHeartbeatTime = now;
	}
}

void MultiplayerManager::sendHeartbeat()
{
	if (m_connectionState == ConnectionState::Disconnected)
	{
		return;
	}

	sendPacket(net::PacketType::Heartbeat, nullptr, 0, true);
}

void MultiplayerManager::sendHandshake()
{
	if (m_connectionState == ConnectionState::Disconnected)
	{
		return;
	}

	if (m_localHandshakeSent)
	{
		return;
	}

	if (m_localNonce == 0)
	{
		m_localNonce = GenerateNonce();
	}

	net::HandshakeMessage handshake{};
	handshake.nonce = m_localNonce;
	handshake.role = (m_role == Role::Host) ? net::ConnectionRole::Host : net::ConnectionRole::Client;

	if (sendPacket(net::PacketType::Handshake, &handshake, static_cast<uint32>(sizeof(handshake)), true))
	{
		m_localHandshakeSent = true;
		Console << U"[MultiplayerManager] Handshake sent";
	}
}

bool MultiplayerManager::sendPacket(net::PacketType type, const void* payload, uint32 payloadSize, bool allowDuringHandshake)
{
	if (!allowDuringHandshake && m_connectionState != ConnectionState::Connected)
	{
		return false;
	}

	if (payloadSize > net::MaxPayloadSize)
	{
		Console << U"[MultiplayerManager] Payload exceeds maximum size";
		return false;
	}

	net::PacketHeader header;
	header.type = static_cast<uint16>(type);
	header.payloadSize = payloadSize;
	header.sequence = (type == net::PacketType::Heartbeat || type == net::PacketType::Handshake) ? 0 : m_nextSendSequence++;
	header.checksum = (payload && payloadSize > 0) ? net::ComputeChecksum(payload, payloadSize) : 0;

	bool success = false;

	if (m_role == Role::Host)
	{
		if (!m_sessionID)
		{
			return false;
		}

		success = m_server.send(&header, sizeof(header), m_sessionID);
		if (success && payloadSize > 0)
		{
			success = m_server.send(payload, payloadSize, m_sessionID);
		}
	}
	else if (m_role == Role::Client)
	{
		success = m_client.send(&header, sizeof(header));
		if (success && payloadSize > 0)
		{
			success = m_client.send(payload, payloadSize);
		}
	}

	if (!success)
	{
		Console << U"[MultiplayerManager] Failed to send packet";
		return false;
	}

	if (type == net::PacketType::Heartbeat)
	{
		m_lastSentHeartbeatTime = Scene::Time();
	}

	return true;
}

net::PacketType MultiplayerManager::peekPacketType() const
{
	if (m_receiveQueue.isEmpty())
	{
		return net::PacketType::Heartbeat;
	}

	return ToPacketType(m_receiveQueue.front().header.type);
}

void MultiplayerManager::discardFrontPacket()
{
	if (!m_receiveQueue.isEmpty())
	{
		m_receiveQueue.pop_front();
	}
}

void MultiplayerManager::resetState()
{
	m_sessionID.reset();
	m_receiveQueue.clear();
	m_connectionState = ConnectionState::Disconnected;
	m_clientConnectSucceeded = false;
	m_connectionTimestamp = 0.0;
	m_lastHeartbeatTime = 0.0;
	m_lastSentHeartbeatTime = 0.0;
	m_nextSendSequence = 1;
	m_lastReceivedSequence = 0;
	m_localNonce = 0;
	m_remoteNonce = 0;
	m_localHandshakeSent = false;
	m_remoteHandshakeReceived = false;
}
