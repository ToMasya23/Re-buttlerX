# pragma once
#include <Siv3D.hpp>

// アクション種別
enum class ActionType : uint8
{
	Attack1 = 0,    // 10ダメージ
	Attack2 = 1,    // 20ダメージ
	Attack3 = 2,    // 15ダメージ
	Attack4 = 3,    // 30ダメージ
	Defend  = 4,    // 防御
	Escape  = 5,    // 逃走
};

// ダメージ定数（既存の値を維持）
namespace DamageValues
{
	constexpr int32 Attack1 = 10;
	constexpr int32 Attack2 = 20;
	constexpr int32 Attack3 = 15;
	constexpr int32 Attack4 = 30;
}

// コスト定数（既存の値を維持）
namespace CostValues
{
	constexpr int32 Attack = 10;
	constexpr int32 Defend = 20;
}

// バトル定数（既存の値を維持）
namespace BattleConstants
{
	constexpr int32 MaxHP = 100;
	constexpr double CostRegenPerSec = 1.0;
	constexpr double DefendDurationSec = 5.0;
	constexpr int32 MaxCrazy = 100;
}
