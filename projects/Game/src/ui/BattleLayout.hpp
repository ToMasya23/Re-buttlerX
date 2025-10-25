# pragma once
# include "../Common.hpp"

namespace BattleLayout
{
	// エンティティ（キャラ）サイズ
	inline constexpr Size EntitySize{ 80, 80 };

	// HP バーサイズ
	inline constexpr double HPBarWidth = 240.0;
	inline constexpr double HPBarHeight = 16.0;
	inline constexpr double HPBarYOffset = 8.0;
	inline constexpr double HPLabelYOffset = 28.0;

	// ボタンサイズ
	inline constexpr Size ButtonSize{ 220, 52 };
	inline constexpr int32 ButtonR = 8;
	inline constexpr double ButtonYOffset = 60.0;
	inline constexpr double ButtonGapHalf = 140.0; // 2 ボタンの左右距離の半分

	inline Vec2 PlayerPos(const Size& sceneSize)
	{
		return Vec2{ 40.0, static_cast<double>(sceneSize.y) - 250.0 };
	}

	inline Vec2 EnemyPos(const Size& sceneSize)
	{
		return Vec2{ static_cast<double>(sceneSize.x) - 250.0, 60.0 };
	}

	inline RectF PlayerHPBarBG(const Size& sceneSize)
	{
		const Vec2 p = PlayerPos(sceneSize);
		return RectF{ p.x, p.y + EntitySize.y + HPBarYOffset, HPBarWidth, HPBarHeight };
	}

	inline RectF EnemyHPBarBG(const Size& sceneSize)
	{
		const Vec2 p = EnemyPos(sceneSize);
		return RectF{ p.x, p.y + EntitySize.y + HPBarYOffset, HPBarWidth, HPBarHeight };
	}

	inline Vec2 PlayerHPLabelPos(const Size& sceneSize)
	{
		const Vec2 p = PlayerPos(sceneSize);
		return Vec2{ p.x, p.y + EntitySize.y + HPLabelYOffset };
	}

	inline Vec2 EnemyHPLabelPos(const Size& sceneSize)
	{
		const Vec2 p = EnemyPos(sceneSize);
		return Vec2{ p.x, p.y + EntitySize.y + HPLabelYOffset };
	}

	inline RoundRect AttackMainButton(const Size& sceneSize)
	{
		return RoundRect{ Arg::center(sceneSize.x / 2 - ButtonGapHalf, sceneSize.y - ButtonYOffset), ButtonSize.x, ButtonSize.y, ButtonR };
	}

	inline RoundRect EscapeMainButton(const Size& sceneSize)
	{
		return RoundRect{ Arg::center(sceneSize.x / 2 + ButtonGapHalf, sceneSize.y - ButtonYOffset), ButtonSize.x, ButtonSize.y, ButtonR };
	}

	inline RoundRect Attack1Button(const Size& sceneSize)
	{
		return AttackMainButton(sceneSize);
	}

	inline RoundRect Attack2Button(const Size& sceneSize)
	{
		return EscapeMainButton(sceneSize);
	}
}


