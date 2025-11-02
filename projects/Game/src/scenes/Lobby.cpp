# include "Lobby.hpp"

// UIButton
void UIButton::draw(const s3d::Font& font) const {
	using namespace s3d;

	const ColorF bg = over ? UI::Accent2 : UI::Accent1;
	rr.draw(bg).drawFrame(5, 0, UI::Frame);

	double textShiftX = 0.0;

	if (icon) {
		// 像素风更清晰
		const ScopedRenderStates2D _nn{ SamplerState::ClampNearest };

		// 目标图标高度：按钮高度 - 上下内边距
		const double iconTargetH = rr.rect.h - padV * 2.0;
		const double s = iconTargetH / icon.height();           // 等比缩放
		const Vec2   pos{ rr.rect.x + padL, rr.rect.y + padV }; // 左上角

		icon.scaled(s).draw(pos);

		// 让文字右移一点（保持整体视觉居中）
		if (centerCompensate) {
			textShiftX = 0.5 * (padL + icon.width() * s + gap);
		}
	}

	// 居中绘制标题；如果有图标，则整体右移一点点
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
	mCharImage = s3d::Texture{ U"assets/ui/characters/主人公.jpg", s3d::TextureDesc::Unmipped };

	recalcLayout(Scene::Size());

	AudioManager::instance().startBGM(U"assets/BGM/menu.mp3", 0.7);
}

Lobby::~Lobby() {
	// 离开 Lobby 停止 BGM（或在下个场景里直接播放新的也行）
	AudioManager::instance().stopBGM();
}

void Lobby::recalcLayout(const Size& size) {
	// 外框
	mOuter = RectF{ 12, 12, size.x - 24.0, size.y - 24.0 };
	mRightX = mOuter.x + mOuter.w - mRightW - 30.0;

	// 右上资料条 + 按钮
	mRightTop = RectF{ mRightX - 70, 25,  mRightW, 100 };
	mBtn1 = UIButton{ RectF{ mRightX, 180, mRightW, 90 }, U"対人戦", mIconPVP, 20 };
	mBtn2 = UIButton{ RectF{ mRightX, 300, mRightW, 90 }, U"練習戦", mIconPVE, 20 };

	// 左侧头像
	mAvatarL = Circle{ mLeftX + 22, mTopY + 22, 18 };

	// 角色占位框（移到中间，不挡左侧文字）
	mCharBox = RectF{ 200, 260, 240, 320 };

	// 右下暂停按钮
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

    /*m_pvpTr.update(m_pvpButton.mouseOver());
    m_pveTr.update(m_pveButton.mouseOver());
    m_exitTr.update(m_exitButton.mouseOver());

    if (m_pvpButton.mouseOver() || m_pveButton.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    if (m_pvpButton.leftClicked())
    {
        getData().lastMode = GameData::GameMode::PvP;
        changeScene(State::Matching);
    }
    else if (m_pveButton.leftClicked())
    {
        getData().lastMode = GameData::GameMode::PvE;
        changeScene(State::Game);
    }*/

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
        //const ScopedRenderTarget2D rt{ m_sceneRT };
        //m_sceneRT.clear(ColorF{ 0.25, 0.25, 0.35 });

        /*FontAsset(U"TitleFont")(U"ロビー")
            .drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.15, 0.2 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 100, Vec2{ 400, 140 });

        m_pvpButton.draw(ColorF{ 1.0, m_pvpTr.value() }).drawFrame(2);
        m_pveButton.draw(ColorF{ 1.0, m_pveTr.value() }).drawFrame(2);

        const Font& bold = FontAsset(U"Bold");
        bold(U"PvP: マッチングへ").drawAt(28, m_pvpButton.center(), ColorF{ 0.1 });
        bold(U"PvE: すぐ開始").drawAt(28, m_pveButton.center(), ColorF{ 0.1 });*/
    }

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(UI::Bg);   // 背景色

		// === ここから “新しいロビーUI” の描画（あなたが既に作った部分）===
		// 全局外框
		mOuter.rounded(22).draw(UI::Panel);
		mOuter.rounded(22).drawFrame(9, 0, UI::Frame);

		// 左侧：头像 + 名称
		mAvatarL.draw(ColorF{ 0.93, 0.95, 1.0 }).drawFrame(5, 0, UI::Frame);
		mTitle(U"マーシャ").draw(mLeftX + 54, mTopY + 4, UI::Text);
		mSmall(U"@Masya23_spl").draw(mLeftX + 54, mTopY + 38, UI::Text);

		// 左侧列表
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

		// 预留一点内边距，避免贴图压到边框
		const s3d::RectF content = mCharBox.stretched(-12);

		// 如果有图就画图，没图仍然画原来的阴影占位
		if (mCharImage)
		{
			// 保持像素风：使用最近邻采样
			const s3d::ScopedRenderStates2D _nn{ s3d::SamplerState::ClampNearest };

			const double sx = content.w / mCharImage.width();
			const double sy = content.h / mCharImage.height();
			const double s = s3d::Min(sx, sy);                 // 等比缩放
			mCharImage.scaled(s).drawAt(content.center());      // 居中绘制
		}

		// 右上资料条
		const RoundRect bar{ mRightTop, 18 };
		bar.draw(UI::Panel);
		const Circle avaR{ bar.rect.x + 28, bar.rect.y + 50, 20 };
		avaR.draw(ColorF{ 0.90, 0.96, 1.0 }).drawFrame(5, 0, UI::Frame);
		mTitle(U"ネットエンジェル").draw(bar.rect.x + 60, bar.rect.y + 16, UI::Text);
		mSmall(U"@ネットエンジ").draw(bar.rect.x + 60, bar.rect.y + 52, UI::Text);

		// 右侧按钮
		mBtn1.draw(mTitle);
		mBtn2.draw(mTitle);

		// 右下暂停按钮（アイコンだけ描画。クリック判定は update 側）
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


