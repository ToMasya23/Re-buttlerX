# pragma once
#include <Siv3D.hpp>

// バトル定数
namespace BattleConstants
{
	static constexpr int32 MaxHP = 100;
	static constexpr double CostRegenPerSec = 1.0;
	static constexpr double DefendDurationSec = 5.0;
	static constexpr double HitDuration = 0.25;
}

// アクションタイプ
enum class ActionType : uint8
{
	None = 0,
	Attack1 = 1,
	Attack2 = 2,
	Attack3 = 3,
	Attack4 = 4,
	Defend = 5,
	Skill = 6,
};
