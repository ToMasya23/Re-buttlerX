# pragma once
# include "../Common.hpp"

namespace PauseTheme
{
	// パネル
	inline constexpr Point PanelCenter{ 400, 310 };
	inline constexpr Size PanelSize{ 520, 540 };
	inline constexpr int32 PanelR = 12;
	inline constexpr ColorF PanelFill{ 0.95, 0.95, 0.96 };
	inline constexpr ColorF PanelFrame{ 0.2, 0.2, 0.3 };

	// タイトル
	inline constexpr Point TitlePos{ 400, 100 };
	inline constexpr int32 TitleFontSize = 64; // FontAssetで指定済
	inline constexpr ColorF TitleColor{ 0.15 };

	// ボタン
	inline constexpr Size ButtonSize{ 240, 52 };
	inline constexpr int32 ButtonR = 8;
	inline constexpr int32 ButtonXs = 400; // center.x
	inline constexpr int32 ButtonYs[6] = { 125, 195, 265, 335, 405, 475 };

	// 暗転
	inline constexpr ColorF Dimmer{ 0.0, 0.35 };
}


