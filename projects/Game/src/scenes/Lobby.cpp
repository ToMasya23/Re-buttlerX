# include "Lobby.hpp"

// UIButton
void UIButton::draw(const s3d::Font& font) const {
	using namespace s3d;

	const ColorF bg = over ? UI::Accent2 : UI::Accent1;
	rr.draw(bg).drawFrame(5, 0, UI::Frame);

	double textShiftX = 0.0;

	if (icon) {
		// ピクセル感を保つための最近傍サンプラ
		const ScopedRenderStates2D _nn{ SamplerState::ClampNearest };

		// アイコン目標高さ：ボタン高さ − 上下パディング
		const double iconTargetH = rr.rect.h - padV * 2.0;
		const double s = iconTargetH / icon.height();           // 等比スケーリング
		const Vec2   pos{ rr.rect.x + padL, rr.rect.y + padV }; // 左上座標

		icon.scaled(s).draw(pos);

		// 視覚中心を揃えるため、テキストを少し右へ補正
		if (centerCompensate) {
			textShiftX = 0.5 * (padL + icon.width() * s + gap);
		}
	}

	// 中央描画（アイコンがある場合はわずかに右へオフセット）
	font(label).drawAt(28, rr.center().movedBy(textShiftX, 0), UI::Text);
}

bool UIButton::update() {
	over = rr.mouseOver();
	if (over) s3d::Cursor::RequestStyle(s3d::CursorStyle::Hand);
	return rr.leftClicked();
}

Lobby::Lobby(const InitData& init)
	: IScene{ init }
{
	mIconPVP = s3d::Texture{ U"assets/ui/PvP.png", TextureDesc::Unmipped };
	mIconPVE = s3d::Texture{ U"assets/ui/PvE.png", TextureDesc::Unmipped };

	recalcLayout(Scene::Size());

	AudioManager::instance().startBGM(U"assets/BGM/menu.mp3", 0.7);
}

Lobby::~Lobby() {
	// ロビー離脱時にBGMを停止（次のシーン側で別曲を再生するなら停止せず切替でも可）
	//AudioManager::instance().stopBGM();
}

void Lobby::recalcLayout(const Size& size) {
	// 外枠
	mOuter = RectF{ 12, 12, size.x - 24.0, size.y - 24.0 };
	mRightX = mOuter.x + mOuter.w - mRightW - 30.0;

	// 右上の情報バー + ボタン
	mRightTop = RectF{ mRightX - 70, 25,  mRightW, 100 };
	mBtn1 = UIButton{ RectF{ mRightX, 180, mRightW, 90 }, U"対人戦", mIconPVP, 20 };
	mBtn2 = UIButton{ RectF{ mRightX, 300, mRightW, 90 }, U"練習戦", mIconPVE, 20 };

	// 左上アバター
	mAvatarL = Circle{ mLeftX + 22, mTopY + 22, 18 };

	// キャラ占位枠（中央寄せで、左側テキストに被らない位置へ）
	mCharBox = RectF{ 200, 260, 240, 280 };

	// 右下の一時停止ボタン
	mPauseBtn = RoundRect{ RectF{ mOuter.x + mOuter.w - 110, mOuter.y + mOuter.h - 80, 90, 64 }, 18 };
}

void Lobby::update()
{
	// Esc でポーズ切り替え
	if (KeyEscape.down())
	{
		m_paused = (not m_paused);
		return;
	}

	if (m_paused)
	{
		m_resumeTr.update(m_resumeButton.mouseOver());
		m_settingsTr.update(m_settingsButton.mouseOver());
		m_howToTr.update(m_howToButton.mouseOver());
		m_effectTr.update(m_effectButton.mouseOver());
		m_titleTr.update(m_titleButton.mouseOver());
		m_exitPauseTr.update(m_exitPauseButton.mouseOver());

		if (m_resumeButton.mouseOver() || m_settingsButton.mouseOver() || m_howToButton.mouseOver()
			|| m_effectButton.mouseOver() || m_titleButton.mouseOver() || m_exitPauseButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (m_resumeButton.leftClicked())
		{
			m_paused = false;
		}
		else if (m_settingsButton.leftClicked())
		{
			changeScene(State::Settings);
		}
		else if (m_howToButton.leftClicked())
		{
			changeScene(State::HowToPlay);
		}
		else if (m_effectButton.leftClicked())
		{
			changeScene(State::EffectViewer);
		}
		else if (m_titleButton.leftClicked())
		{
			changeScene(State::Title);
		}
		else if (m_exitPauseButton.leftClicked())
		{
			System::Exit();
		}

		return;
	}

	// 旧UIの更新は未使用のため省略

	const bool b1 = mBtn1.update();
	const bool b2 = mBtn2.update();

	if (mBtn1.over || mBtn2.over || mPauseBtn.mouseOver()) {
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	if (b1) {
		// 旧 PvP/PvE の遷移に合わせて、ここでシーン遷移
		getData().lastMode = GameData::GameMode::PvP;
		changeScene(State::Matching);
	}
	else if (b2) {
		getData().lastMode = GameData::GameMode::PvE;
		changeScene(State::Game);
	}

	// 右下の一時停止ボタン（非ポーズ時のみ有効）
	if (mPauseBtn.leftClicked()) {
		m_paused = true;
	}
}

void Lobby::draw() const
{
	const Size sceneSize = Scene::Size();

	if ((not m_sceneRT) || (m_sceneRT.size() != sceneSize))
	{
		const_cast<Lobby*>(this)->m_sceneRT = RenderTexture{ sceneSize };
	}
	if ((not m_blurInternal) || (m_blurInternal.size() != sceneSize))
	{
		const_cast<Lobby*>(this)->m_blurInternal = RenderTexture{ sceneSize };
	}
	if ((not m_blurTarget) || (m_blurTarget.size() != sceneSize))
	{
		const_cast<Lobby*>(this)->m_blurTarget = RenderTexture{ sceneSize };
	}

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(UI::Bg);   // 背景色

		// === 新しいロビー UI の描画 ===
		// 全体外枠
		mOuter.rounded(22).draw(UI::Panel);
		mOuter.rounded(22).drawFrame(9, 0, UI::Frame);

		// 左側：アバター + 名前
		mAvatarL.draw(ColorF{ 0.93, 0.95, 1.0 }).drawFrame(5, 0, UI::Frame);
		mTitle(U"マーシャ").draw(mLeftX + 54, mTopY + 4, UI::Text);
		mSmall(U"@Masya23_spl").draw(mLeftX + 54, mTopY + 38, UI::Text);

		// 左側メニュー
		const double listX = mLeftX + 8;
		const double listY = 110.0;
		const double lh = 34.0;
		const Array<String> items{
			U"プロフィール", U"ワザーらん", U"せつめい",
			U"せってい", U"りれき", U"タイトル"
		};
		for (size_t i = 0; i < items.size(); ++i) {
			const double y = listY + i * lh;
			Circle{ listX + 8, y + 10, 5 }.draw(UI::Frame);
			mUI(items[i]).draw(listX + 24, y, UI::Text);
		}

		// キャラ占位（画像に差し替える場合は下2行を Texture.fitted(...).drawAt(...) に置換）
		const RoundRect box{ mCharBox, 16 };
		box.draw(ColorF{ 0.90, 0.94, 1.0 });
		box.drawFrame(5, 0, UI::Frame);
		Ellipse{ mCharBox.center().movedBy(0, mCharBox.h * 0.55), 90, 12 }
		.draw(ColorF(0, 0, 0, 0.12));

		// 右上の情報バー
		const RoundRect bar{ mRightTop, 18 };
		bar.draw(UI::Panel);
		const Circle avaR{ bar.rect.x + 28, bar.rect.y + 50, 20 };
		avaR.draw(ColorF{ 0.90, 0.96, 1.0 }).drawFrame(5, 0, UI::Frame);
		mTitle(U"ネットエンジェル").draw(bar.rect.x + 60, bar.rect.y + 16, UI::Text);
		mSmall(U"@ネットエンジ").draw(bar.rect.x + 60, bar.rect.y + 52, UI::Text);

		// 右側のボタン
		mBtn1.draw(mTitle);
		mBtn2.draw(mTitle);

		// 右下：ポーズボタン（アイコンのみ描画。クリック判定は update 側）
		mPauseBtn.draw(ColorF{ 0.96, 0.92, 1.0 }).drawFrame(5, 0, UI::Frame);
		{
			const Vec2 c = mPauseBtn.center();
			RectF{ c.x - 14, c.y - 18, 8, 36 }.rounded(4).draw(UI::Text);
			RectF{ c.x + 6,  c.y - 18, 8, 36 }.rounded(4).draw(UI::Text);
		}
		// === ここまで ===
	}

	if (m_paused)
	{
		Shader::GaussianBlur(m_sceneRT, m_blurInternal, m_blurTarget, BoxFilterSize::BoxFilter9x9);
		m_blurTarget.draw();
		Rect{ sceneSize }.draw(PauseTheme::Dimmer);

		const Font& title = FontAsset(U"TitleFont");
		const Font& bold = FontAsset(U"Bold");
		const RoundRect panel{ Arg::center(PauseTheme::PanelCenter), PauseTheme::PanelSize, PauseTheme::PanelR };
		panel.draw(PauseTheme::PanelFill).drawFrame(3, 0, PauseTheme::PanelFrame);
		title(U"PAUSE").drawAt(64, Vec2{ PauseTheme::TitlePos }, PauseTheme::TitleColor);

		m_resumeButton.draw(ColorF{ 1.0, m_resumeTr.value() }).drawFrame(2);
		m_settingsButton.draw(ColorF{ 1.0, m_settingsTr.value() }).drawFrame(2);
		m_howToButton.draw(ColorF{ 1.0, m_howToTr.value() }).drawFrame(2);
		m_effectButton.draw(ColorF{ 1.0, m_effectTr.value() }).drawFrame(2);
		m_titleButton.draw(ColorF{ 1.0, m_titleTr.value() }).drawFrame(2);
		m_exitPauseButton.draw(ColorF{ 1.0, m_exitPauseTr.value() }).drawFrame(2);

		bold(U"再開").drawAt(28, m_resumeButton.center(), ColorF{ 0.1 });
		bold(U"設定").drawAt(28, m_settingsButton.center(), ColorF{ 0.1 });
		bold(U"ゲーム説明").drawAt(28, m_howToButton.center(), ColorF{ 0.1 });
		bold(U"効果確認").drawAt(28, m_effectButton.center(), ColorF{ 0.1 });
		bold(U"タイトルへ").drawAt(28, m_titleButton.center(), ColorF{ 0.1 });
		bold(U"EXIT").drawAt(28, m_exitPauseButton.center(), ColorF{ 0.1 });
	}
	else
	{
		m_sceneRT.draw();
	}
}
