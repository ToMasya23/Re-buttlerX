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

// ===== クレイジーモード時のグリッチエフェクト描画 =====
inline void drawGlitchEffect(const s3d::Texture& tex, const s3d::RectF& dst, const s3d::ColorF& tint = s3d::Palette::White)
{
	if (!tex) return;

	constexpr double ChangeInterval = 0.1; // 何秒ごとにグリッチ内容を更新するか

	// スロット番号をシードにして 0.1 秒ごとに固定の乱数列を生成
	const uint64 slot = static_cast<uint64>(Math::Floor(Scene::Time() / ChangeInterval));
	DefaultRNG rng{ slot * 1000003ULL };
	std::uniform_real_distribution<double> dist01{ 0.0, 1.0 };

	const double sc = Min(dst.w / tex.width(), dst.h / tex.height());
	const Vec2 drawPos = dst.center() - Vec2{ tex.width(), tex.height() } * sc * 0.5;

	// 通常描画
	drawFit(tex, dst, tint);

	// ===== チャンネルシフト（色収差）=====
	// 通常描画の上に R・B チャンネルをずらして加算合成することで色ずれを演出する
	{
		const double shiftX = (dist01(rng) * 2.0 - 1.0) * 8.0;
		const double shiftY = (dist01(rng) * 2.0 - 1.0) * 4.0;

		ScopedRenderStates2D blend{ BlendState::Additive };
		ScopedRenderStates2D _nn{ SamplerState::ClampNearest };
		tex.scaled(sc).draw(drawPos + Vec2{  shiftX,  shiftY }, ColorF{ tint.r, 0.0,    0.0,    0.5 });
		tex.scaled(sc).draw(drawPos + Vec2{ -shiftX, -shiftY }, ColorF{ 0.0,    0.0,    tint.b, 0.5 });
	}

	// ===== 横帯ずれ =====
	if (dist01(rng) > 0.8) return; // 80% の確率でグリッチ発生

	constexpr int32 MaxBandHeight = 30; // 帯の最大高さ（テクスチャピクセル単位）

	ScopedRenderStates2D _nn{ SamplerState::ClampNearest };
	for (int32 i = 0; i < 5; ++i)
	{
		// テクスチャの横帯をランダムに切り出し、拡大して元の帯位置に重ねて描画
		const int32 ry = static_cast<int32>(dist01(rng) * (tex.height() - 1));
		const int32 maxRh = Min(MaxBandHeight, tex.height() - ry);
		const int32 rh = Max(1, static_cast<int32>(dist01(rng) * maxRh));
		const Rect srcRect{ 0, ry, tex.width(), rh };

		// ランダムな拡大率（1.1～1.5倍）
		const double scaleFactor = 1.1 + dist01(rng) * 0.4;
		const double drawW = tex.width() * sc * scaleFactor;
		const double drawH = rh            * sc * scaleFactor;

		// 元の帯の中心に合わせて配置
		const double bandCenterX = drawPos.x + tex.width() * sc * 0.5;
		const double bandCenterY = drawPos.y + ry * sc + rh * sc * 0.5;
		const Vec2 pos{ bandCenterX - drawW * 0.5, bandCenterY - drawH * 0.5 };

		tex(srcRect).resized(drawW, drawH).draw(pos, ColorF{ tint.r, tint.g, tint.b, 0.6 });
	}
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
