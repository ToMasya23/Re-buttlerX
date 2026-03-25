#pragma once
// Game の内部実装ファイル間で共有するユーティリティ
// 各 Game_*.cpp が #include "Game_Impl.hpp" することで利用できる

#include "Game.hpp"
#include "../tools/NineSlice.hpp"
#include <fstream>

// ネットワークログファイル（Game.cpp で定義）
extern std::ofstream g_networkLog;

// ===== 内部定数 =====
inline constexpr int32 Damage1 = 10;
inline constexpr int32 Damage2 = 20;

// ===== 画面フレーム =====
inline NineSliceSkin& ScreenFrame()
{
	static NineSliceSkin skin{
		U"assets/ui/frames/battle_frame.png",
		20, 20, 20, 20,
		false
	};
	return skin;
}

inline NineSliceSkin& BaseFrame()
{
	static NineSliceSkin skin{
		U"assets/ui/frames/battle_base.png",
		20, 20, 20, 20,
		false
	};
	return skin;
}

// ===== テクスチャをアスペクト比を保ってフィットさせて描画 =====
inline void drawFit(const s3d::Texture& tex, const s3d::RectF& dst, const s3d::ColorF& tint = s3d::Palette::White)
{
	const s3d::ScopedRenderStates2D _nn{ s3d::SamplerState::ClampNearest };
	const double sx = dst.w / tex.width();
	const double sy = dst.h / tex.height();
	const double s = s3d::Min(sx, sy);
	const s3d::Vec2 size = s3d::Vec2{ tex.width(), tex.height() } * s;
	const s3d::Vec2 pos = dst.center() - size * 0.5;
	tex.scaled(s).draw(pos, tint);
}

// ===== スロット番号を ActionType に変換 =====
inline ActionType actionTypeFromSlot(int slot)
{
	switch (slot)
	{
	case 0: return ActionType::Attack1;
	case 1: return ActionType::Attack2;
	case 2: return ActionType::Attack3;
	case 3: return ActionType::Attack4;
	default: return ActionType::Attack1;
	}
}
