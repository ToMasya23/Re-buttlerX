# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../tools/AudioManager.hpp"

namespace UI {
	inline constexpr s3d::ColorF Bg{ 0.975, 0.965, 0.985 };
	inline constexpr s3d::ColorF Panel{ 1.00,  0.98,  1.00 };
	inline constexpr s3d::ColorF Frame{ 0.40,  0.32,  0.60 };
	inline constexpr s3d::ColorF Accent1{ 0.70,  0.80,  0.98 };
	inline constexpr s3d::ColorF Accent2{ 0.82,  0.88,  1.00 };
	inline constexpr s3d::ColorF Text{ 0.18,  0.12,  0.28 };
}

//——— 簡易ボタン（オプションで左側アイコン付き）——//
struct UIButton {
	s3d::RoundRect rr;
	s3d::String    label;
	s3d::Texture   icon;                 // オプション：空の場合はアイコンを描画しない
	bool           over = false;

	// ビジュアルパラメータ
	double radius = 20.0;
	double padL = 18.0;               // 左パディング（アイコンとボタン左端の距離）
	double padV = 12.0;               // 上下パディング（アイコンの目標高さを決定）
	double gap = 6.0;               // アイコンとテキストの間隔
	bool   centerCompensate = true;     // テキストを右にずらし、アイコンの視覚的重みを相殺するか

	UIButton() = default;

	// 旧用法（アイコンなし）も引き続き使用可能
	UIButton(const s3d::RectF& r, const s3d::String& text, double rads = 20.0)
		: rr{ r, rads }, label{ text }, radius{ rads } {
	}

	// 新用法（アイコン付き）
	UIButton(const s3d::RectF& r, const s3d::String& text, const s3d::Texture& iconTex, double rads = 20.0)
		: rr{ r, rads }, label{ text }, icon{ iconTex }, radius{ rads } {
	}

	// 実行時にアイコンを設定/変更
	void setIcon(const s3d::Texture& t) { icon = t; }

	// 描画と操作
	void draw(const s3d::Font& font) const;
	bool update(); // クリックされたかどうかを返す
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
	s3d::Texture m_player{ U"assets/ui/characters/player.png" };

	Transition m_pvpTr{ 0.4s, 0.2s };
	Transition m_pveTr{ 0.4s, 0.2s };
	Transition m_exitTr{ 0.4s, 0.2s };

	// ---- ポーズ用 ----
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

	// フォント
	s3d::Font mTitle{ 34, s3d::Typeface::Bold };
	s3d::Font mUI{ 24 };
	s3d::Font mSmall{ 18 };

	// レイアウト
	s3d::RectF mOuter;
	double mLeftX = 36.0;
	double mTopY = 34.0;

	double mRightW = 350.0;
	double mRightX = 0.0;
	s3d::RectF mRightTop;

	UIButton mBtn1;
	UIButton mBtn2;

	s3d::Texture mIconPVP;       // 対人戦
	s3d::Texture mIconPVE;       // 練習戦

	s3d::Circle mAvatarL;
	s3d::RectF  mCharBox;

	s3d::RoundRect mPauseBtn{ s3d::RectF{ 0,0,90,64 }, 18 };
};



