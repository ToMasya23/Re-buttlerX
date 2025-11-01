# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../tools/AudioManager.hpp"

// UI カラー定義
namespace UI {
	inline constexpr s3d::ColorF Bg{ 0.975, 0.965, 0.985 };     // 画面背景
	inline constexpr s3d::ColorF Panel{ 1.00,  0.98,  1.00 };   // パネル面
	inline constexpr s3d::ColorF Frame{ 0.40,  0.32,  0.60 };   // 枠線
	inline constexpr s3d::ColorF Accent1{ 0.70,  0.80,  0.98 }; // ボタン通常
	inline constexpr s3d::ColorF Accent2{ 0.82,  0.88,  1.00 }; // ボタンホバー
	inline constexpr s3d::ColorF Text{ 0.18,  0.12,  0.28 };    // テキスト色
}

//——— 簡易ボタン（左側アイコンは任意）——//
struct UIButton {
	s3d::RoundRect rr;     // ボタンの描画領域（角丸）
	s3d::String    label;  // 表示テキスト
	s3d::Texture   icon;   // 任意：空ならアイコンを描かない
	bool           over = false; // ホバー状態

	// 見た目パラメータ
	double radius = 20.0;  // 角丸半径
	double padL = 18.0;    // 左パディング（アイコンの左余白）
	double padV = 12.0;    // 縦パディング（アイコンの目標高さに影響）
	double gap = 6.0;      // アイコンと文字の間隔
	bool   centerCompensate = true; // 視覚的重心補正（アイコン分だけ文字を少し右へ）

	UIButton() = default;

	// 旧来の使い方（アイコンなし）
	UIButton(const s3d::RectF& r, const s3d::String& text, double rads = 20.0)
		: rr{ r, rads }, label{ text }, radius{ rads } {
	}

	// 新しい使い方（アイコンあり）
	UIButton(const s3d::RectF& r, const s3d::String& text, const s3d::Texture& iconTex, double rads = 20.0)
		: rr{ r, rads }, label{ text }, icon{ iconTex }, radius{ rads } {
	}

	// 実行時にアイコンを設定／差し替え
	void setIcon(const s3d::Texture& t) { icon = t; }

	// 描画と入力処理
	void draw(const s3d::Font& font) const;
	bool update(); // クリックされたら true を返す
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

	// 旧ボタン（未使用のためコメント化）
	//RoundRect m_pvpButton{ Arg::center(400, 260), 300, 60, 8 };
	//RoundRect m_pveButton{ Arg::center(400, 340), 300, 60, 8 };
	//RoundRect m_exitButton{ Arg::center(400, 420), 300, 60, 8 };

	// ホバー演出（トランジション）
	Transition m_pvpTr{ 0.4s, 0.2s };
	Transition m_pveTr{ 0.4s, 0.2s };
	Transition m_exitTr{ 0.4s, 0.2s };

	// ---- ポーズ関連 ----
	bool m_paused = false;         // ポーズ中か
	RenderTexture m_sceneRT;       // ぼかし用レンダーターゲット
	RenderTexture m_blurInternal;  // ガウシアンブラー内部
	RenderTexture m_blurTarget;    // ブラー結果

	// ポーズメニューのボタン
	RoundRect m_resumeButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[0]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_settingsButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[1]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_howToButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[2]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_effectButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[3]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_titleButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[4]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_exitPauseButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[5]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };

	// ポーズメニューボタン用トランジション
	Transition m_resumeTr{ 0.3s, 0.15s };
	Transition m_settingsTr{ 0.3s, 0.15s };
	Transition m_howToTr{ 0.3s, 0.15s };
	Transition m_effectTr{ 0.3s, 0.15s };
	Transition m_titleTr{ 0.3s, 0.15s };
	Transition m_exitPauseTr{ 0.3s, 0.15s };

	// レイアウト再計算（ウィンドウサイズ変化時などに呼ぶ）
	void recalcLayout(const s3d::Size& size);

	// フォント
	s3d::Font mTitle{ 34, s3d::Typeface::Bold }; // タイトル用
	s3d::Font mUI{ 24 };                         // UI本文
	s3d::Font mSmall{ 18 };                      // 補助テキスト

	// レイアウト矩形
	s3d::RectF mOuter;   // 画面内の外枠
	double mLeftX = 36.0;
	double mTopY = 34.0;

	double mRightW = 250.0; // 右側パネルの幅
	double mRightX = 0.0;   // 右側パネルの X（recalcLayout で算出）
	s3d::RectF mRightTop;   // 右上情報パネル

	// 右側の大きいボタン（対人戦／練習戦）
	UIButton mBtn1;
	UIButton mBtn2;

	// ボタン用アイコン（対人戦／練習戦）
	s3d::Texture mIconPVP; // 対人戦アイコン
	s3d::Texture mIconPVE; // 練習戦アイコン

	// 左側：プロフィール表示など
	s3d::Circle mAvatarL;  // 左上アバター
	s3d::RectF  mCharBox;  // 画面中央寄りのキャラ占位枠

	// 右下：ポーズボタン
	s3d::RoundRect mPauseBtn{ s3d::RectF{ 0,0,90,64 }, 18 };
};
