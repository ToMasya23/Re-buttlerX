# include "MultiplayerManager.hpp"

MultiplayerManager::MultiplayerManager()
{
}

bool MultiplayerManager::startHost(uint16 port)
{
	m_server.startAccept(port);
	m_role = Role::Host;
	return true;
}

bool MultiplayerManager::connect(const s3d::IPv4Address& address)
{
	if (m_client.connect(address, 12345))
	{
		m_role = Role::Client;
		return true;
	}
	return false;
}

void MultiplayerManager::disconnect()
{
	m_client.disconnect();
	m_server.cancelAccept();
	m_sessionID.reset();
	m_role = Role::None;
	m_receiveQueue.clear();
}

bool MultiplayerManager::isConnected() const
{
	if (m_role == Role::Host)
	{
		return m_sessionID.has_value() && m_server.hasSession(*m_sessionID);
	}
	else
	{
		return m_client.isConnected();
	}
}

void MultiplayerManager::update()
{
	// ホストの場合、接続待ち受け
	if (m_role == Role::Host && !m_sessionID)
	{
		if (m_server.hasSession())
		{
			auto sessions = m_server.getSessionIDs();
			if (!sessions.isEmpty())
			{
				m_sessionID = sessions.front();
			}
		}
	}

	processIncomingData();
}

void MultiplayerManager::processIncomingData()
{
	if (!isConnected())
		return;

	uint32 size = 0;

	if (m_role == Role::Host)
	{
		while (m_server.available(m_sessionID) >= sizeof(size))
		{
			if (!m_server.lookahead(size, m_sessionID))
				break;

			if (size == 0 || size > 1024 * 1024)
				break;

			if (m_server.available(m_sessionID) < sizeof(size) + size)
				break;

			if (!m_server.read(&size, sizeof(size), m_sessionID))
				break;

			s3d::Blob blob(size);
			if (m_server.read(blob.data(), size, m_sessionID) && blob.size() == size)
			{
				m_receiveQueue.push_back(blob);
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		while (m_client.available() >= sizeof(size))
		{
			if (!m_client.lookahead(size))
				break;

			if (size == 0 || size > 1024 * 1024)
				break;

			if (m_client.available() < sizeof(size) + size)
				break;

			if (!m_client.read(&size, sizeof(size)))
				break;

			s3d::Blob blob(size);
			if (m_client.read(blob.data(), size) && blob.size() == size)
			{
				m_receiveQueue.push_back(blob);
			}
			else
			{
				break;
			}
		}
	}
}

s3d::Optional<MessageType> MultiplayerManager::peekMessageType()
{
	if (m_receiveQueue.isEmpty())
		return s3d::none;

	const auto& blob = m_receiveQueue.front();

	if (blob.isEmpty() || blob.size() < 1)
		return s3d::none;

	const uint8* data = reinterpret_cast<const uint8*>(blob.data());
	MessageType type = static_cast<MessageType>(data[0]);

	return type;
}


// シリアライゼーション（PlayerActionMessage用）
template<>
void MultiplayerManager::send<PlayerActionMessage>(const PlayerActionMessage& message)
{
	if (!isConnected())
		return;

	s3d::Array<uint8> data;
	data.push_back(static_cast<uint8>(message.type));
	data.push_back(static_cast<uint8>(message.action));

	uint32 turnNum = message.turnNumber;
	data.push_back((turnNum >> 0) & 0xFF);
	data.push_back((turnNum >> 8) & 0xFF);
	data.push_back((turnNum >> 16) & 0xFF);
	data.push_back((turnNum >> 24) & 0xFF);

	const uint32 size = static_cast<uint32>(data.size());

	if (m_role == Role::Host)
	{
		m_server.send(&size, sizeof(size), m_sessionID);
		m_server.send(data.data(), data.size(), m_sessionID);
	}
	else
	{
		m_client.send(&size, sizeof(size));
		m_client.send(data.data(), data.size());
	}
}

template<typename T>
void MultiplayerManager::send(const T& message)
{
	// Unsupported message type
}

// デシリアライゼーション（PlayerActionMessage用）
template<>
s3d::Optional<PlayerActionMessage> MultiplayerManager::receive<PlayerActionMessage>()
{
	if (m_receiveQueue.isEmpty())
		return s3d::none;

	auto blob = m_receiveQueue.front();
	m_receiveQueue.pop_front();

	if (blob.size() < 6)
		return s3d::none;

	const uint8* data = reinterpret_cast<const uint8*>(blob.data());

	PlayerActionMessage message;
	message.type = static_cast<MessageType>(data[0]);
	message.action = static_cast<ActionType>(data[1]);
	message.turnNumber = (static_cast<uint32>(data[2]) << 0) |
	                     (static_cast<uint32>(data[3]) << 8) |
	                     (static_cast<uint32>(data[4]) << 16) |
	                     (static_cast<uint32>(data[5]) << 24);

	return message;
}

template<typename T>
s3d::Optional<T> MultiplayerManager::receive()
{
	if (!m_receiveQueue.isEmpty())
		m_receiveQueue.pop_front();
	return s3d::none;
}

// GameStateSyncMessage の特殊化
template<>
void MultiplayerManager::send<GameStateSyncMessage>(const GameStateSyncMessage& message)
{
	if (!isConnected())
		return;

	s3d::Array<uint8> data;
	data.push_back(static_cast<uint8>(message.type));

	// Serialize all fields (int32, double, bool)
	auto writeInt32 = [&](int32 val) {
		data.push_back((val >> 0) & 0xFF);
		data.push_back((val >> 8) & 0xFF);
		data.push_back((val >> 16) & 0xFF);
		data.push_back((val >> 24) & 0xFF);
	};

	auto writeDouble = [&](double val) {
		const uint64 bits = *reinterpret_cast<const uint64*>(&val);
		for (int i = 0; i < 8; ++i)
			data.push_back((bits >> (i * 8)) & 0xFF);
	};

	auto writeBool = [&](bool val) {
		data.push_back(val ? 1 : 0);
	};

	writeInt32(message.hostHP);
	writeDouble(message.hostCost);
	writeBool(message.hostDefending);
	writeDouble(message.hostDefendTime);
	writeInt32(message.hostCrazy);

	writeInt32(message.clientHP);
	writeDouble(message.clientCost);
	writeBool(message.clientDefending);
	writeDouble(message.clientDefendTime);
	writeInt32(message.clientCrazy);

	writeBool(message.isHostTurn);
	writeInt32(static_cast<int32>(message.turnNumber));

	const uint32 size = static_cast<uint32>(data.size());

	if (m_role == Role::Host)
	{
		m_server.send(&size, sizeof(size), m_sessionID);
		m_server.send(data.data(), data.size(), m_sessionID);
	}
	else
	{
		m_client.send(&size, sizeof(size));
		m_client.send(data.data(), data.size());
	}
}

template<>
s3d::Optional<GameStateSyncMessage> MultiplayerManager::receive<GameStateSyncMessage>()
{
	if (m_receiveQueue.isEmpty())
		return s3d::none;

	auto blob = m_receiveQueue.front();
	m_receiveQueue.pop_front();

	if (blob.size() < 56)
		return s3d::none;

	const uint8* data = reinterpret_cast<const uint8*>(blob.data());
	size_t offset = 0;

	GameStateSyncMessage message;
	message.type = static_cast<MessageType>(data[offset++]);

	auto readInt32 = [&]() -> int32 {
		int32 val = (static_cast<int32>(data[offset + 0]) << 0) |
		            (static_cast<int32>(data[offset + 1]) << 8) |
		            (static_cast<int32>(data[offset + 2]) << 16) |
		            (static_cast<int32>(data[offset + 3]) << 24);
		offset += 4;
		return val;
	};

	auto readDouble = [&]() -> double {
		uint64 bits = 0;
		for (int i = 0; i < 8; ++i)
			bits |= (static_cast<uint64>(data[offset++]) << (i * 8));
		return *reinterpret_cast<const double*>(&bits);
	};

	auto readBool = [&]() -> bool {
		return data[offset++] != 0;
	};

	message.hostHP = readInt32();
	message.hostCost = readDouble();
	message.hostDefending = readBool();
	message.hostDefendTime = readDouble();
	message.hostCrazy = readInt32();

	message.clientHP = readInt32();
	message.clientCost = readDouble();
	message.clientDefending = readBool();
	message.clientDefendTime = readDouble();
	message.clientCrazy = readInt32();

	message.isHostTurn = readBool();
	message.turnNumber = static_cast<uint32>(readInt32());

	return message;
}

// TurnChangeMessage の特殊化
template<>
void MultiplayerManager::send<TurnChangeMessage>(const TurnChangeMessage& message)
{
	if (!isConnected())
		return;

	s3d::Array<uint8> data;
	data.push_back(static_cast<uint8>(message.type));
	data.push_back(message.isHostTurn ? 1 : 0);

	uint32 turnNum = message.turnNumber;
	data.push_back((turnNum >> 0) & 0xFF);
	data.push_back((turnNum >> 8) & 0xFF);
	data.push_back((turnNum >> 16) & 0xFF);
	data.push_back((turnNum >> 24) & 0xFF);

	const uint32 size = static_cast<uint32>(data.size());
	if (m_role == Role::Host)
	{
		m_server.send(&size, sizeof(size), m_sessionID);
		m_server.send(data.data(), data.size(), m_sessionID);
	}
	else
	{
		m_client.send(&size, sizeof(size));
		m_client.send(data.data(), data.size());
	}
}

template<>
s3d::Optional<TurnChangeMessage> MultiplayerManager::receive<TurnChangeMessage>()
{
	if (m_receiveQueue.isEmpty())
		return s3d::none;

	auto blob = m_receiveQueue.front();
	m_receiveQueue.pop_front();

	if (blob.size() < 6)
		return s3d::none;

	const uint8* data = reinterpret_cast<const uint8*>(blob.data());

	TurnChangeMessage message;
	message.type = static_cast<MessageType>(data[0]);
	message.isHostTurn = (data[1] != 0);
	message.turnNumber = (static_cast<uint32>(data[2]) << 0) |
	                     (static_cast<uint32>(data[3]) << 8) |
	                     (static_cast<uint32>(data[4]) << 16) |
	                     (static_cast<uint32>(data[5]) << 24);

	return message;
}

// BattleTextMessage の特殊化
template<>
void MultiplayerManager::send<BattleTextMessage>(const BattleTextMessage& message)
{
	if (!isConnected())
		return;

	s3d::Array<uint8> data;
	data.push_back(static_cast<uint8>(message.type));

	// String を UTF-8 でエンコード
	std::string utf8 = message.message.toUTF8();
	uint32 strLen = static_cast<uint32>(utf8.size());
	data.push_back((strLen >> 0) & 0xFF);
	data.push_back((strLen >> 8) & 0xFF);
	data.push_back((strLen >> 16) & 0xFF);
	data.push_back((strLen >> 24) & 0xFF);

	for (char c : utf8)
		data.push_back(static_cast<uint8>(c));

	const uint32 size = static_cast<uint32>(data.size());
	if (m_role == Role::Host)
	{
		m_server.send(&size, sizeof(size), m_sessionID);
		m_server.send(data.data(), data.size(), m_sessionID);
	}
	else
	{
		m_client.send(&size, sizeof(size));
		m_client.send(data.data(), data.size());
	}
}

template<>
s3d::Optional<BattleTextMessage> MultiplayerManager::receive<BattleTextMessage>()
{
	if (m_receiveQueue.isEmpty())
		return s3d::none;

	auto blob = m_receiveQueue.front();
	m_receiveQueue.pop_front();

	if (blob.size() < 5)
		return s3d::none;

	const uint8* data = reinterpret_cast<const uint8*>(blob.data());

	BattleTextMessage message;
	message.type = static_cast<MessageType>(data[0]);

	uint32 strLen = (static_cast<uint32>(data[1]) << 0) |
	                (static_cast<uint32>(data[2]) << 8) |
	                (static_cast<uint32>(data[3]) << 16) |
	                (static_cast<uint32>(data[4]) << 24);

	if (blob.size() < 5 + strLen)
		return s3d::none;

	std::string utf8(reinterpret_cast<const char*>(data + 5), strLen);
	message.message = Unicode::FromUTF8(utf8);

	return message;
}

// BattleEndMessage の特殊化
template<>
void MultiplayerManager::send<BattleEndMessage>(const BattleEndMessage& message)
{
	if (!isConnected())
		return;

	s3d::Array<uint8> data;
	data.push_back(static_cast<uint8>(message.type));
	data.push_back(message.hostWon ? 1 : 0);

	auto writeInt32 = [&](int32 val) {
		data.push_back((val >> 0) & 0xFF);
		data.push_back((val >> 8) & 0xFF);
		data.push_back((val >> 16) & 0xFF);
		data.push_back((val >> 24) & 0xFF);
	};

	writeInt32(message.hostFinalHP);
	writeInt32(message.clientFinalHP);
	data.push_back(message.endReason);

	const uint32 size = static_cast<uint32>(data.size());
	if (m_role == Role::Host)
	{
		m_server.send(&size, sizeof(size), m_sessionID);
		m_server.send(data.data(), data.size(), m_sessionID);
	}
	else
	{
		m_client.send(&size, sizeof(size));
		m_client.send(data.data(), data.size());
	}
}

template<>
s3d::Optional<BattleEndMessage> MultiplayerManager::receive<BattleEndMessage>()
{
	if (m_receiveQueue.isEmpty())
		return s3d::none;

	auto blob = m_receiveQueue.front();
	m_receiveQueue.pop_front();

	if (blob.size() < 11)  // 1 + 1 + 4 + 4 + 1
		return s3d::none;

	const uint8* data = reinterpret_cast<const uint8*>(blob.data());

	BattleEndMessage message;
	message.type = static_cast<MessageType>(data[0]);
	message.hostWon = (data[1] != 0);

	auto readInt32 = [&](size_t offset) -> int32 {
		return (static_cast<int32>(data[offset + 0]) << 0) |
		       (static_cast<int32>(data[offset + 1]) << 8) |
		       (static_cast<int32>(data[offset + 2]) << 16) |
		       (static_cast<int32>(data[offset + 3]) << 24);
	};

	message.hostFinalHP = readInt32(2);
	message.clientFinalHP = readInt32(6);
	message.endReason = data[10];

	return message;
}
