# include "Matching.hpp"

Matching::Matching(const InitData& init)
	: IScene{ init }
{
	m_ipInput.text = U"192.168.1.100";  // デフォルトをローカルネットワークのサンプルIPに
	m_multiplayer = std::make_shared<MultiplayerManager>();
}

void Matching::update()
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

	// 接続待機中の処理
	if (m_isWaitingForConnection)
	{
		m_multiplayer->update();
		
		if (m_multiplayer->isConnected())
		{
			// 接続成功、ゲームシーンへ
			getData().multiplayer = m_multiplayer;
			getData().isHost = m_isHost;
			getData().lastMode = GameData::GameMode::PvP;
			changeScene(State::Game);
		}
		
		if (m_backButton.leftClicked())
		{
			m_multiplayer->disconnect();
			m_isWaitingForConnection = false;
		}
		
		m_backTr.update(m_backButton.mouseOver());
		if (m_backButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		
		return;
	}

    m_hostTr.update(m_hostButton.mouseOver());
    m_joinTr.update(m_joinButton.mouseOver());
    m_backTr.update(m_backButton.mouseOver());

    if (m_hostButton.mouseOver() || m_joinButton.mouseOver() || m_backButton.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    if (m_hostButton.leftClicked())
    {
		// ホストとして開始（全てのネットワークインターフェースでリッスン）
		m_multiplayer->startHost(12345);
		m_isHost = true;
		m_isWaitingForConnection = true;
		
		Console << U"[ホスト] ポート12345で接続待機中...";
		Console << U"[ホスト] 自分のIPアドレスを相手に伝えてください";
    }
    else if (m_joinButton.leftClicked())
    {
		// クライアントとして接続（入力されたIPアドレスを使用）
		Console << U"[クライアント] " << m_ipInput.text << U":12345 に接続中...";
		
		// Parse IP address (例: "192.168.1.100")
		Array<String> parts = m_ipInput.text.split(U'.');
		if (parts.size() == 4)
		{
			try
			{
				uint8 a = Parse<uint8>(parts[0]);
				uint8 b = Parse<uint8>(parts[1]);
				uint8 c = Parse<uint8>(parts[2]);
				uint8 d = Parse<uint8>(parts[3]);
				
				s3d::IPv4Address addr{ a, b, c, d };
				
				if (m_multiplayer->connect(addr, 12345))
				{
					m_isHost = false;
					m_isWaitingForConnection = true;
					Console << U"[クライアント] 接続開始";
				}
				else
				{
					Console << U"[クライアント] 接続失敗";
				}
			}
			catch (...)
			{
				Console << U"[クライアント] IPアドレスのパースに失敗";
			}
		}
		else
		{
			Console << U"[クライアント] IPアドレスが無効です (形式: 192.168.1.100)";
		}
    }
    else if (m_backButton.leftClicked())
    {
        changeScene(State::Lobby);
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

		if (m_isWaitingForConnection)
		{
			FontAsset(U"TitleFont")(m_isHost ? U"接続待機中..." : U"接続中...")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 72, Vec2{ 400, 200 });
			
			// ホストの場合、IPアドレス確認方法を表示
			if (m_isHost)
			{
				const Font& bold = FontAsset(U"Bold");
				bold(U"相手に伝える情報:").drawAt(24, Vec2{ 400, 280 }, ColorF{ 0.8 });
				bold(U"ポート番号: 12345").drawAt(28, Vec2{ 400, 320 }, ColorF{ 1.0, 1.0, 0.5 });
				bold(U"IPアドレスの確認方法:").drawAt(20, Vec2{ 400, 360 }, ColorF{ 0.7 });
				bold(U"Windowsキー + R → 'cmd' → 'ipconfig'").drawAt(18, Vec2{ 400, 390 }, ColorF{ 0.6 });
				bold(U"「IPv4アドレス」を相手に伝えてください").drawAt(18, Vec2{ 400, 415 }, ColorF{ 0.6 });
			}
			
			m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);
			const Font& bold = FontAsset(U"Bold");
			bold(U"キャンセル").drawAt(28, m_backButton.center(), ColorF{ 0.1 });
		}
		else
		{
			FontAsset(U"TitleFont")(U"オンライン対戦")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 72, Vec2{ 400, 150 });

			// IP入力フィールドを表示（参加する用）
			const Font& bold = FontAsset(U"Bold");
			bold(U"接続先IPアドレス (例: 192.168.1.100):").drawAt(20, Vec2{ 400, 205 }, ColorF{ 0.8 });
			
			// IP入力（参加ボタンを押す前のみ編集可能）
			SimpleGUI::TextBox(const_cast<s3d::TextEditState&>(m_ipInput), Vec2{ 200, 220 }, 400);

			m_hostButton.draw(ColorF{ 1.0, m_hostTr.value() }).drawFrame(2);
			m_joinButton.draw(ColorF{ 1.0, m_joinTr.value() }).drawFrame(2);
			m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);

			bold(U"ホストとして開始").drawAt(28, m_hostButton.center(), ColorF{ 0.1 });
			bold(U"参加する").drawAt(28, m_joinButton.center(), ColorF{ 0.1 });
			bold(U"ロビーに戻る").drawAt(28, m_backButton.center(), ColorF{ 0.1 });
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


