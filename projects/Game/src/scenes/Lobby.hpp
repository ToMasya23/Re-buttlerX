# pragma once
# include "../Common.hpp"
#include "../ui/PauseMenu.hpp"
#include "../ui/PauseTheme.hpp"
# include "../tools/AudioManager.hpp"

namespace UI {
	inline constexpr s3d::ColorF Bg{ 0.975, 0.965, 0.985 };
	inline constexpr s3d::ColorF Panel{ 1.00,  0.98,  1.00 };
	inline constexpr s3d::ColorF Frame{ 0.40,  0.32,  0.60 };
	inline constexpr s3d::ColorF Accent1{ 0.70,  0.80,  0.98 };
	inline constexpr s3d::ColorF Accent2{ 0.82,  0.88,  1.00 };
	inline constexpr s3d::ColorF Text{ 0.18,  0.12,  0.28 };
}

//——— 简易按钮（带可选左侧图标）——//
struct UIButton {
	s3d::RoundRect rr;
	s3d::String    label;
	s3d::Texture   icon;                 // 可选：为空则不画图标
	bool           over = false;

	// 视觉参数
	double radius = 20.0;
	double padL = 18.0;               // 左内边距（图标距离按钮左侧）
	double padV = 12.0;               // 上下内边距（决定图标目标高度）
	double gap = 6.0;               // 图标与文字的间距
	bool   centerCompensate = true;     // 是否让文字右移一点，抵消图标的视觉重量

	UIButton() = default;

	// 旧用法（无图标）仍可用
	UIButton(const s3d::RectF& r, const s3d::String& text, double rads = 20.0)
		: rr{ r, rads }, label{ text }, radius{ rads } {
	}

	// 新用法（带图标）
	UIButton(const s3d::RectF& r, const s3d::String& text, const s3d::Texture& iconTex, double rads = 20.0)
		: rr{ r, rads }, label{ text }, icon{ iconTex }, radius{ rads } {
	}

	// 运行时设置/更换图标
	void setIcon(const s3d::Texture& t) { icon = t; }

	// 绘制与交互
	void draw(const s3d::Font& font) const;
	bool update(); // 返回是否被点击
};

// ロビーシーン
class Lobby : public App::Scene
{
public:

	Lobby(const InitData& init);

	~Lobby();

	void update() override;

	void draw() const override;

private:

	//RoundRect m_pvpButton{ Arg::center(400, 260), 300, 60, 8 };
	//RoundRect m_pveButton{ Arg::center(400, 340), 300, 60, 8 };
	//RoundRect m_exitButton{ Arg::center(400, 420), 300, 60, 8 };

	Transition m_pvpTr{ 0.4s, 0.2s };
	Transition m_pveTr{ 0.4s, 0.2s };
	Transition m_exitTr{ 0.4s, 0.2s };

	// ---- ポーズ用 ----
	void updatePausedUI();
	PauseMenu m_pauseMenu;

	bool m_paused = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

	RoundRect m_resumeButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[0]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_settingsButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[1]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_howToButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[2]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_effectButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[3]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_titleButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[4]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_exitPauseButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[5]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };

	Transition m_resumeTr{ 0.3s, 0.15s };
	Transition m_settingsTr{ 0.3s, 0.15s };
	Transition m_howToTr{ 0.3s, 0.15s };
	Transition m_effectTr{ 0.3s, 0.15s };
	Transition m_titleTr{ 0.3s, 0.15s };
	Transition m_exitPauseTr{ 0.3s, 0.15s };

	void recalcLayout(const s3d::Size& size);

	// 字体
	s3d::Font mTitle{ 34, s3d::Typeface::Bold };
	s3d::Font mUI{ 24 };
	s3d::Font mSmall{ 18 };

	// 布局
	s3d::RectF mOuter;
	double mLeftX = 36.0;
	double mTopY = 34.0;

	double mRightW = 250.0;
	double mRightX = 0.0;
	s3d::RectF mRightTop;

	UIButton mBtn1;
	UIButton mBtn2;

	s3d::Texture mIconPVP;       // 对人战
	s3d::Texture mIconPVE;       // 练习战

	s3d::Circle mAvatarL;
	s3d::RectF  mCharBox;

	s3d::RoundRect mPauseBtn{ s3d::RectF{ 0,0,90,64 }, 18 };
};


