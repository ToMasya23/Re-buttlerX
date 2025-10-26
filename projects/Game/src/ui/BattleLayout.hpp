# pragma once
# include "../Common.hpp"

namespace BattleLayout
{
	// エンティティ（キャラ）サイズ
	inline constexpr Size EntitySize{ 80, 80 };

	// HP バーサイズ（頭上配置）
	inline constexpr double HPBarWidth = 240.0;
	inline constexpr double HPBarHeight = 16.0;
	inline constexpr double HPBarAboveOffset = 12.0; // 頭上余白
	inline constexpr double HPLabelAboveOffset = 34.0; // バー上のラベル距離

	// ボタンサイズ
	inline constexpr Size ButtonSize{ 220, 52 };
	inline constexpr int32 ButtonR = 8;
	inline constexpr double ButtonYOffset = 60.0; // 画面下ボタンの基準（逃げる）
	inline constexpr double ButtonGapHalf = 140.0; // 互換用

	// 攻撃選択（左上に縦並び）
	inline constexpr Size AttackButtonSize{ 160, 70 };
	inline constexpr int32 AttackButtonR = 8;
	inline constexpr double AttackLeftMargin = 20.0;
	inline constexpr double AttackTopMargin = 100.0;
	inline constexpr double AttackButtonGap = 20.0;

	// 左上スペース：コストボックス
	inline constexpr Size CostPanelSize{ 250, 60 };
	inline constexpr int32 CostPanelR = 8;
	inline constexpr double CostLeftMargin = 20.0;
	inline constexpr double CostTopMargin = 20.0;
	inline RectF CostPanelRect(const Size& /*sceneSize*/)
	{
		return RectF{ CostLeftMargin, CostTopMargin, static_cast<double>(CostPanelSize.x), static_cast<double>(CostPanelSize.y) };
	}

	inline Vec2 PlayerPos(const Size& sceneSize)
	{
		return Vec2{ 270.0, static_cast<double>(sceneSize.y) - 250.0 };
	}

	inline Vec2 EnemyPos(const Size& sceneSize)
	{
		return Vec2{ static_cast<double>(sceneSize.x) - 290.0, 70.0 };
	}

	inline RectF PlayerHPBarBG(const Size& sceneSize)
	{
		const Vec2 p = PlayerPos(sceneSize);
		return RectF{ p.x, p.y - HPBarAboveOffset - HPBarHeight, HPBarWidth, HPBarHeight };
	}

	inline RectF EnemyHPBarBG(const Size& sceneSize)
	{
		const Vec2 p = EnemyPos(sceneSize);
		return RectF{ p.x, p.y - HPBarAboveOffset - HPBarHeight, HPBarWidth, HPBarHeight };
	}

	inline Vec2 PlayerHPLabelPos(const Size& sceneSize)
	{
		const Vec2 p = PlayerPos(sceneSize);
		return Vec2{ p.x, p.y - HPBarAboveOffset - HPBarHeight - HPLabelAboveOffset };
	}

	inline Vec2 EnemyHPLabelPos(const Size& sceneSize)
	{
		const Vec2 p = EnemyPos(sceneSize);
		return Vec2{ p.x, p.y - HPBarAboveOffset - HPBarHeight - HPLabelAboveOffset };
	}

// クレイジーゲージ（HPバー右端に配置）
inline constexpr double CrazyRingRadius = 20.0;
inline constexpr double CrazyRingOffsetX = 18.0; // HPバー右端からのオフセット
inline Vec2 PlayerCrazyCenter(const Size& sceneSize)
{
    const RectF bar = PlayerHPBarBG(sceneSize);
    return Vec2{ bar.x + bar.w + CrazyRingOffsetX, bar.y + bar.h * 0.5 };
}
inline Vec2 EnemyCrazyCenter(const Size& sceneSize)
{
    const RectF bar = EnemyHPBarBG(sceneSize);
    return Vec2{ bar.x + bar.w + CrazyRingOffsetX, bar.y + bar.h * 0.5 };
}

	// 左上：攻撃1〜4 ボタン
	inline RoundRect AttackOptionButton(const Size& sceneSize, int index)
	{
		const double x = AttackLeftMargin + AttackButtonSize.x * 0.5;
		const double y = AttackTopMargin + (AttackButtonSize.y + AttackButtonGap) * index + AttackButtonSize.y * 0.5;
		return RoundRect{ Arg::center(x, y), AttackButtonSize.x, AttackButtonSize.y, AttackButtonR };
	}

	// 左下：プレイヤーパネル（アイコン＋防御）
	inline constexpr Size PlayerPanelSize{ 240, 88 };
	inline constexpr int32 PlayerPanelR = 10;
	inline RectF PlayerPanelRect(const Size& sceneSize)
	{
		return RectF{ 16, static_cast<double>(sceneSize.y) - PlayerPanelSize.y - 16, static_cast<double>(PlayerPanelSize.x), static_cast<double>(PlayerPanelSize.y) };
	}
	inline RectF PlayerIconRect(const RectF& panel)
	{
		return RectF{ panel.x + 12, panel.y + 12, 64, 64 };
	}
	inline RoundRect DefendButtonRect(const RectF& panel)
	{
		return RoundRect{ RectF{ panel.x + 12 + 64 + 12, panel.y + 12, panel.w - 64 - 12 - 24, 64 }, 8 };
	}

	// 右端：逃げるボタン
	inline RoundRect EscapeButton(const Size& sceneSize)
	{
		return RoundRect{ Arg::center(sceneSize.x - 120, sceneSize.y - ButtonYOffset), ButtonSize.x, ButtonSize.y, ButtonR };
	}
}


