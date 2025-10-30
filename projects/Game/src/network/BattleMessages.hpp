# pragma once
#include <Siv3D.hpp>
#include "../game/BattleTypes.hpp"

// メッセージタイプ
enum class MessageType : uint8
{
	PlayerAction    = 0x01,  // プレイヤーアクション
	GameStateSync   = 0x11,  // 状態同期
	TurnChange      = 0x12,  // ターン変更
	BattleMessage   = 0x13,  // テキストメッセージ
	BattleEnd       = 0x14,  // 戦闘終了
};

// プレイヤーアクションメッセージ
struct PlayerActionMessage
{
	MessageType type = MessageType::PlayerAction;
	ActionType action = ActionType::Attack1;
	int32 damage = 0;  // ダメージ量
	uint32 turnNumber = 0;
};

// ゲーム状態同期メッセージ
struct GameStateSyncMessage
{
	MessageType type = MessageType::GameStateSync;

	// ホスト側の状態
	int32 hostHP = 0;
	double hostCost = 0.0;
	bool hostDefending = false;
	double hostDefendTime = 0.0;
	int32 hostCrazy = 0;

	// クライアント側の状態
	int32 clientHP = 0;
	double clientCost = 0.0;
	bool clientDefending = false;
	double clientDefendTime = 0.0;
	int32 clientCrazy = 0;

	// バトル状態
	bool isHostTurn = false;
	uint32 turnNumber = 0;
};

// ターン変更メッセージ
struct TurnChangeMessage
{
	MessageType type = MessageType::TurnChange;
	bool isHostTurn = false;
	uint32 turnNumber = 0;
};

// バトルテキストメッセージ
struct BattleTextMessage
{
	MessageType type = MessageType::BattleMessage;
	String message;
};

// 戦闘終了メッセージ
struct BattleEndMessage
{
	MessageType type = MessageType::BattleEnd;
	bool hostWon = false;
	int32 hostFinalHP = 0;
	int32 clientFinalHP = 0;
	uint8 endReason = 0;  // 0=HP0, 1=逃走, 2=切断
};
