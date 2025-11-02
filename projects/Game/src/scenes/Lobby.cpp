# include "Lobby.hpp"

// UIButton
void UIButton::draw(const s3d::Font& font) const {
	using namespace s3d;

	const ColorF bg = over ? UI::Accent2 : UI::Accent1;
	rr.draw(bg).drawFrame(5, 0, UI::Frame);

	double textShiftX = 0.0;

	if (icon) {
		// ピクセル風でより鮮明に
		const ScopedRenderStates2D _nn{ SamplerState::ClampNearest };

		// 目標アイコン高さ：ボタン高さ - 上下パディング
		const double iconTargetH = rr.rect.h - padV * 2.0;
		const double s = iconTargetH / icon.height();           // 等比拡大縮小
		const Vec2   pos{ rr.rect.x + padL, rr.rect.y + padV }; // 左上隅

		icon.scaled(s).draw(pos);

		// テキストを右に少しずらす（全体の視覚的中央を保つ）
		if (centerCompensate) {
			textShiftX = 0.5 * (padL + icon.width() * s + gap);
		}
	}

	// 中央にタイトルを描画；アイコンがあれば全体を少し右にずらす
	font(label).drawAt(36, rr.center().movedBy(textShiftX, 0), UI::Text);
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
	// Lobby を離れる際に BGM を停止（または次のシーンで新しい BGM を直接再生してもよい）
	AudioManager::instance().stopBGM();
}

void Lobby::recalcLayout(const Size& size) {
	using namespace s3d;
	
	// 外枠
	mOuter = RectF{ 12, 12, size.x - 24.0, size.y - 24.0 };
	mRightX = mOuter.x + mOuter.w - mRightW - 30.0;

	// 右上情報バー + ボタン
	mRightTop = RectF{ mRightX - 90, 25,  mRightW, 100 };
	mBtn1 = UIButton{ RectF{ mRightX, 180, mRightW, 110 }, U"対人戦", mIconPVP, 25 };
	mBtn2 = UIButton{ RectF{ mRightX, 320, mRightW, 110 }, U"練習戦", mIconPVE, 25 };

	// 左側アバター
	mAvatarL = Circle{ mLeftX + 22, mTopY + 22, 18 };

	// キャラクタープレースホルダー枠（中央に移動し、左側のテキストを遮らない）
	mCharBox = RectF{ 200, 260, 240, 280 };

	// 右下一時停止ボタン
	mPauseBtn = RoundRect{ RectF{ mOuter.x + mOuter.w - 110, mOuter.y + mOuter.h - 80, 90, 64 }, 18 };
}

void Lobby::update()
{
    // Esc トグル
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

	const bool b1 = mBtn1.update();
	const bool b2 = mBtn2.update();

	if (mBtn1.over || mBtn2.over || mPauseBtn.mouseOver()) {
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	if (b1) {
		// 旧 PvP/PvE 遷移に合わせるならここでハンドリング
		getData().lastMode = GameData::GameMode::PvP;
		changeScene(State::Matching);
	}
	else if (b2) {
		getData().lastMode = GameData::GameMode::PvE;
		changeScene(State::Game);
	}

	// 右下の一時停止ボタン（未ポーズ時のみ有効）
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
		using namespace s3d;
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(UI::Bg);   // 背景色

		// === ここから "新しいロビーUI" の描画 ===
		// 全体の外枠
		mOuter.rounded(22).draw(UI::Panel);
		mOuter.rounded(22).drawFrame(9, 0, UI::Frame);

		// 左側：アバター + 名前
		//mAvatarL.draw(ColorF{ 0.93, 0.95, 1.0 }).drawFrame(5, 0, UI::Frame);
		//mTitle(U"マーシャ").draw(mLeftX + 54, mTopY + 4, UI::Text);
		//mSmall(U"@Masya23_spl").draw(mLeftX + 54, mTopY + 38, UI::Text);

		// 左側リスト
		//const double listX = mLeftX + 8;
		//const double listY = 110.0;
		//const double lh = 34.0;
		//const Array<String> items{
		//	U"プロフィール", U"ワザーらん", U"せつめい",
		//	U"せってい", U"りれき", U"タイトル"
		//};
		//for (size_t i = 0; i < items.size(); ++i) {
		//	const double y = listY + i * lh;
		//	Circle{ listX + 8, y + 10, 5 }.draw(UI::Frame);
		//	mUI(items[i]).draw(listX + 24, y, UI::Text);
		//}

		// キャラクタープレースホルダー（テクスチャ化する際は以下を Texture.fitted(...).drawAt(...) に置き換える）
		//s3d::RoundRect(mCharBox, 16).draw(ColorF{ 0.90, 0.94, 1.0 });
		//s3d::RoundRect(mCharBox, 16).drawFrame(5, 0, UI::Frame);
		//s3d::Ellipse(mCharBox.center().movedBy(0, mCharBox.h * 0.55), 90, 12).draw(ColorF(0, 0, 0, 0.12));
		//m_player.fitted(mCharBox).drawAt(mCharBox.center());
		m_player.resized(300, 600).drawAt(Vec2{ 200, 330 });

		// 右上情報バー
		const RoundRect bar{ mRightTop, 18 };
		bar.draw(UI::Panel);
		const Circle avaR{ bar.rect.x + 80, bar.rect.y + 55, 20 };
		avaR.draw(ColorF{ 0.90, 0.96, 1.0 }).drawFrame(5, 0, UI::Frame);
		mTitle(U"ネットエンジェル").draw(bar.rect.x + 110, bar.rect.y + 30, UI::Text);
		mSmall(U"@ネットエンジ").draw(bar.rect.x + 110, bar.rect.y + 80, UI::Text);

		// 右側ボタン
		mBtn1.draw(mTitle);
		mBtn2.draw(mTitle);

		// 右下一時停止ボタン（アイコンだけ描画。クリック判定は update 側）
		mPauseBtn.draw(ColorF{ 0.96, 0.92, 1.0 }).drawFrame(5, 0, UI::Frame);
		{
			const Vec2 c = mPauseBtn.center();
			RectF{ c.x - 14, c.y - 18, 8, 36 }.rounded(4).draw(UI::Text);
			RectF{ c.x + 6,  c.y - 18, 8, 36 }.rounded(4).draw(UI::Text);
		}
		// === 新 UI 描画ここまで ===
	}

    if (m_paused)
    {
        Shader::GaussianBlur(m_sceneRT, m_blurInternal, m_blurTarget, BoxFilterSize::BoxFilter9x9);
        m_blurTarget.draw();
        Rect{ sceneSize }.draw(PauseTheme::Dimmer);

        const Font& title = FontAsset(U"TitleFont");
        const Font& bold = FontAsset(U"Bold");
        const RoundRect panel{ Arg::center(PauseTheme::PanelCenter), PauseTheme::PanelSize, PauseTheme::PanelR };

        m_resumeButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_resumeTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
        m_settingsButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_settingsTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
        m_howToButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_howToTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
        m_effectButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_effectTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
        m_titleButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_titleTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
        m_exitPauseButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_exitPauseTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });

        bold(U"再開").drawAt(28, m_resumeButton.center(), ColorF{ 1 });
        bold(U"設定").drawAt(28, m_settingsButton.center(), ColorF{ 1 });
        bold(U"ゲーム説明").drawAt(28, m_howToButton.center(), ColorF{ 1 });
        bold(U"効果確認").drawAt(28, m_effectButton.center(), ColorF{ 1 });
        bold(U"タイトルへ").drawAt(28, m_titleButton.center(), ColorF{ 1 });
        bold(U"EXIT").drawAt(28, m_exitPauseButton.center(), ColorF{ 1 });
    }
    else
    {
        m_sceneRT.draw();
    }
}


