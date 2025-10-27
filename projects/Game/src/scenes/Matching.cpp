# include "Matching.hpp"

Matching::Matching(const InitData& init)
	: IScene{ init }
	, m_multiplayer(std::make_shared<MultiplayerManager>())
{
	m_ipInput.text = U"127.0.0.1";  // デフォルトIP
}

void Matching::update()
{
    // Esc で戻る
    if (KeyEscape.down())
    {
        if (!m_paused)
        {
            m_multiplayer->disconnect();
            changeScene(State::Lobby);
            return;
        }
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

    // ===== オンライン対戦の状態管理 =====
    switch (m_matchState)
    {
    case MatchState::SelectMode:
        // IP入力欄の更新
        SimpleGUI::TextBox(m_ipInput, Vec2{ 250, 180 }, 300);

        m_hostTr.update(m_hostButton.mouseOver());
        m_joinTr.update(m_joinButton.mouseOver());
        m_startTr.update(m_startButton.mouseOver());
        m_backTr.update(m_backButton.mouseOver());

        if (m_hostButton.mouseOver() || m_joinButton.mouseOver() ||
            m_startButton.mouseOver() || m_backButton.mouseOver())
        {
            Cursor::RequestStyle(CursorStyle::Hand);
        }

        // ホストボタン
        if (m_hostButton.leftClicked())
        {
            if (m_multiplayer->startHost(12345))
            {
                m_matchState = MatchState::Connecting;
                m_statusMessage = U"接続を待っています...";
                getData().isHost = true;
            }
            else
            {
                m_statusMessage = U"ホスト開始に失敗しました";
            }
        }

        // 参加ボタン
        if (m_joinButton.leftClicked())
        {
            // IPv4Address は StringView を受け取るコンストラクタがある
            s3d::IPv4Address address{ m_ipInput.text };

            if (m_multiplayer->connect(address))
            {
                m_matchState = MatchState::Connecting;
                m_statusMessage = U"接続中...";
                getData().isHost = false;
            }
            else
            {
                m_statusMessage = U"接続に失敗しました";
            }
        }

        // ローカルモック（すぐ開始）
        if (m_startButton.leftClicked())
        {
            getData().lastMode = GameData::GameMode::PvP;
            getData().multiplayer = nullptr;  // オフライン
            changeScene(State::Game);
        }

        // 戻るボタン
        if (m_backButton.leftClicked())
        {
            changeScene(State::Lobby);
        }
        break;

    case MatchState::Connecting:
        m_multiplayer->update();

        if (m_multiplayer->isConnected())
        {
            m_matchState = MatchState::Connected;
            m_statusMessage = U"接続成功！ゲーム開始...";
            getData().multiplayer = m_multiplayer;
            getData().lastMode = GameData::GameMode::PvP;

            // 少し待ってから遷移
            System::Sleep(1s);
            changeScene(State::Game);
        }
        break;

    case MatchState::Connected:
        // 遷移待ち
        break;
    }
}

void Matching::draw() const
{
    const Size sceneSize = Scene::Size();

    if ((not m_sceneRT) || (m_sceneRT.size() != sceneSize))
    {
        const_cast<Matching*>(this)->m_sceneRT = RenderTexture{ sceneSize };
    }
    if ((not m_blurInternal) || (m_blurInternal.size() != sceneSize))
    {
        const_cast<Matching*>(this)->m_blurInternal = RenderTexture{ sceneSize };
    }
    if ((not m_blurTarget) || (m_blurTarget.size() != sceneSize))
    {
        const_cast<Matching*>(this)->m_blurTarget = RenderTexture{ sceneSize };
    }

    {
        const ScopedRenderTarget2D rt{ m_sceneRT };
        m_sceneRT.clear(ColorF{ 0.2, 0.2, 0.2 });

        const Font& bold = FontAsset(U"Bold");

        if (m_matchState == MatchState::SelectMode)
        {
            FontAsset(U"TitleFont")(U"オンライン対戦")
                .drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 56, Vec2{ 400, 100 });

            // IP入力欄
            bold(U"接続先IP:").draw(24, Vec2{ 250, 150 });
            SimpleGUI::TextBox(const_cast<Matching*>(this)->m_ipInput, Vec2{ 250, 180 }, 300);

            // オンライン対戦ボタン
            m_hostButton.draw(ColorF{ 0.3, 0.6, 0.3, 0.8 + m_hostTr.value() * 0.2 }).drawFrame(2);
            bold(U"ホストとして開始").drawAt(24, m_hostButton.center(), ColorF{ 1.0 });

            m_joinButton.draw(ColorF{ 0.3, 0.5, 0.8, 0.8 + m_joinTr.value() * 0.2 }).drawFrame(2);
            bold(U"参加する").drawAt(24, m_joinButton.center(), ColorF{ 1.0 });

            // ローカルモック
            m_startButton.draw(ColorF{ 0.5, 0.5, 0.5, 0.8 + m_startTr.value() * 0.2 }).drawFrame(2);
            bold(U"ローカル開始（モック）").drawAt(20, m_startButton.center(), ColorF{ 1.0 });

            m_backButton.draw(ColorF{ 0.6, 0.3, 0.3, 0.8 + m_backTr.value() * 0.2 }).drawFrame(2);
            bold(U"ロビーに戻る").drawAt(24, m_backButton.center(), ColorF{ 1.0 });

            // ステータスメッセージ
            if (!m_statusMessage.isEmpty())
            {
                bold(m_statusMessage).drawAt(24, Vec2{ 400, 540 }, ColorF{ 1.0, 0.9, 0.2 });
            }
        }
        else if (m_matchState == MatchState::Connecting)
        {
            FontAsset(U"TitleFont")(m_statusMessage)
                .drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 48, Vec2{ 400, 250 });

            // アニメーション（点滅）
            const double t = Scene::Time();
            const int32 dotCount = static_cast<int32>(t * 2) % 4;
            const String dots = String(U".", dotCount);
            bold(dots).drawAt(32, Vec2{ 400, 320 });
        }
        else if (m_matchState == MatchState::Connected)
        {
            FontAsset(U"TitleFont")(U"接続成功！")
                .drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 56, Vec2{ 400, 300 });
        }
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


