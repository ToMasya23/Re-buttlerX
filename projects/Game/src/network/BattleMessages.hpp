#pragma once
#include <Siv3D.hpp>
#include <cstdint>
#include <cstddef>
#include "../game/BattleTypes.hpp"

namespace net
{
	constexpr uint32 PacketMagic = 0x52425858; // "RBXX"
	constexpr uint16 ProtocolVersion = 0x0003;
	constexpr uint32 MaxPayloadSize = 2048;

	enum class PacketType : uint16
	{
		Handshake      = 0x0001,
		Heartbeat      = 0x0002,
		ActionRequest  = 0x0100,
		StateSnapshot  = 0x0101,
		BattleEvent    = 0x0102,
		BattleEnd      = 0x0103,
	};

	enum class ConnectionRole : uint8
	{
		Host = 0,
		Client = 1,
	};

	enum class BattleEventType : uint16
	{
		None = 0,
		HostAttackDamage = 1,
		HostAttackBlocked = 2,
		HostActionRejected = 3,
		HostDefend = 4,
		HostEscape = 5,
		ClientAttackDamage = 6,
		ClientAttackBlocked = 7,
		ClientActionRejected = 8,
		ClientDefend = 9,
		TurnChanged = 10,
		ClientEscape = 11,
		Custom = 0xFFFF,
	};

	enum class BattleEndReason : uint8
	{
		HPZero = 0,
		Escape = 1,
		Disconnect = 2,
		Timeout = 3,
	};

	constexpr uint32 EventFlagRequiresAcknowledge = 1u << 0;
	constexpr uint32 EventFlagHitPlayer = 1u << 1;
	constexpr uint32 EventFlagHitEnemy = 1u << 2;

	struct PacketHeader
	{
		uint32 magic = PacketMagic;
		uint16 version = ProtocolVersion;
		uint16 type = 0;
		uint32 payloadSize = 0;
		uint32 sequence = 0;
		uint32 checksum = 0;
	};

	struct HandshakeMessage
	{
		uint32 version = ProtocolVersion;
		uint32 nonce = 0;
		ConnectionRole role = ConnectionRole::Host;
		uint8 reserved[3]{};
	};

	struct ActionRequestMessage
	{
		uint32 turnNumber = 0;
		uint32 requestId = 0;
		ActionType action = ActionType::Attack1;
		uint8 slotIndex = 0;
		uint8 reserved[3]{};
	};

	struct StateSnapshotMessage
	{
		int32 hostHP = 0;
		int32 clientHP = 0;
		float hostCost = 0.0f;
		float clientCost = 0.0f;
		float hostDefendTime = 0.0f;
		float clientDefendTime = 0.0f;
		int32 hostCrazy = 0;
		int32 clientCrazy = 0;
		uint8 hostDefending = 0;
		uint8 clientDefending = 0;
		uint8 isHostTurn = 0;
		uint8 reserved = 0;
		uint32 turnNumber = 0;
		int32 hostAttributeId = 0;    // ホストの属性ID
		int32 clientAttributeId = 0;  // クライアントの属性ID
	};

	struct BattleEventMessage
	{
		BattleEventType eventType = BattleEventType::None;
		int32 primaryValue = 0;
		int32 secondaryValue = 0;
		uint32 flags = 0;
	};

	struct BattleEndMessage
	{
		int32 hostFinalHP = 0;
		int32 clientFinalHP = 0;
		uint8 hostWon = 0;
		BattleEndReason reason = BattleEndReason::HPZero;
		uint8 reserved[2]{};
	};

	inline uint32 ComputeChecksum(const void* data, size_t size)
	{
		const uint8* bytes = static_cast<const uint8*>(data);
		uint32 hash = 2166136261u; // FNV-1a
		for (size_t i = 0; i < size; ++i)
		{
			hash ^= bytes[i];
			hash *= 16777619u;
		}
		return hash;
	}
}
