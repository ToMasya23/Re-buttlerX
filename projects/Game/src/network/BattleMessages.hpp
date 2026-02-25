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
		ClientHandSync = 0x0104, // クライアント手札同期（Client→Host）
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

	// flags のビット 8-15 に属性ID (0-255) を格納するユーティリティ
	// 攻撃イベント (HostAttackDamage 等) で使用し、受信側が属性IDを復元する
	constexpr uint32 EventFlagAttributeIdShift = 8;
	constexpr uint32 EventFlagAttributeIdMask  = 0xFFu << EventFlagAttributeIdShift;

	inline uint32 packAttributeId(uint32 flags, int32 attrId)
	{
		return (flags & ~EventFlagAttributeIdMask)
			| ((static_cast<uint32>(attrId) & 0xFF) << EventFlagAttributeIdShift);
	}

	inline int32 unpackAttributeId(uint32 flags)
	{
		return static_cast<int32>((flags >> EventFlagAttributeIdShift) & 0xFF);
	}

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
		uint8 cardAttributeId = 0;        // 視覚カードの属性ID (0=デフォルト, 1=量, 2=質, 3=反撃)
		uint8 visualCardPoolIndex = 0xFF; // 視覚カードのプールインデックス (0xFF=無効)
		uint8 actualCardPoolIndex = 0xFF; // 実際カードのプールインデックス (0xFF=無効)
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

	// クライアントの手札状態をホストへ通知するメッセージ (Client→Host)
	// 各スロットの実カード・視覚カードのプールインデックス (0xFF = 空/無効)
	struct ClientHandSyncMessage
	{
		uint8 actualIndices[4] = {0xFF, 0xFF, 0xFF, 0xFF};
		uint8 visualIndices[4] = {0xFF, 0xFF, 0xFF, 0xFF};
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
