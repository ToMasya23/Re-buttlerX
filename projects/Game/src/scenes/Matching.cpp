# include "Matching.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

Matching::Matching(const InitData& init)
	: IScene{ init }
{
	m_multiplayer = std::make_shared<MultiplayerManager>();
	m_hostDiscovery = std::make_unique<HostDiscovery>();
	
	// IP入力のデフォルト値を設定
	m_ipInputState.text = U"192.168.1.100";
	m_ipInputState.cursorPos = m_ipInputState.text.size();
}

IPv4Address Matching::detectLocalIPForDisplay()
{
#ifdef _WIN32
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
	{
		return IPv4Address{ 127, 0, 0, 1 };
	}
	
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	struct addrinfo* result = nullptr;
	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
	{
		return IPv4Address{ 127, 0, 0, 1 };
	}
	
	for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		if (ptr->ai_family == AF_INET)
		{
			struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
			uint32_t addr = ntohl(sockaddr_ipv4->sin_addr.s_addr);
			
			uint8 a = (addr >> 24) & 0xFF;
			uint8 b = (addr >> 16) & 0xFF;
			uint8 c = (addr >> 8) & 0xFF;
			uint8 d = (addr >> 0) & 0xFF;
			
			if (a != 127)
			{
				freeaddrinfo(result);
				return IPv4Address{ a, b, c, d };
			}
		}
	}
	
	freeaddrinfo(result);
#endif
	
	return IPv4Address{ 127, 0, 0, 1 };
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

	// ホスト検索を更新
	if (m_hostDiscovery)
	{
		m_hostDiscovery->update();
	}

	// 接続待機中の処理
	if (m_viewMode == ViewMode::Waiting)
	{
		m_multiplayer->update();
		
		// デバッグ：接続状態を定期的にログ出力
		static double lastCheckTime = 0;
		if (Scene::Time() - lastCheckTime > 1.0)
		{
			bool connected = m_multiplayer->isConnected();
			if (m_isHost)
			{
				Console << U"[ホスト] 接続待機中... isConnected=" << connected;
			}
			else
			{
				Console << U"[クライアント] 接続確認中... isConnected=" << connected;
			}
			lastCheckTime = Scene::Time();
		}
		
		if (m_multiplayer->isConnected())
		{
			// 接続成功、ゲームシーンへ
			if (m_isHost)
			{
				Console << U"[ホスト] クライアントとの接続が確立されました。Game画面に遷移します。";
			}
			else
			{
				Console << U"[クライアント] ホストとの接続が確立されました。Game画面に遷移します。";
			}
			
			m_hostDiscovery->stop();
			getData().multiplayer = m_multiplayer;
			getData().isHost = m_isHost;
			getData().lastMode = GameData::GameMode::PvP;
			changeScene(State::Game);
			return;
		}
		
		if (m_backButton.leftClicked())
		{
			m_multiplayer->disconnect();
			m_hostDiscovery->stop();
			m_viewMode = ViewMode::Menu;
		}
		
		m_backTr.update(m_backButton.mouseOver());
		if (m_backButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		
		return;
	}
	
	// ホストリスト表示中（手動IP入力に変更）
	if (m_viewMode == ViewMode::HostList)
	{
		// テキスト入力の処理
		m_ipInputState.active = true;
		
		// TextInputからの入力を取得
		const String input = TextInput::GetRawInput();
		
		// 数字とドットのみを追加
		for (size_t i = 0; i < input.length(); ++i)
		{
			const char32 ch = input[i];
			if ((ch >= U'0' && ch <= U'9') || ch == U'.')
			{
				m_ipInputState.text += ch;
				m_ipInputState.cursorPos++;
			}
		}
		
		// バックスペース
		if (KeyBackspace.down() && m_ipInputState.cursorPos > 0)
		{
			m_ipInputState.text.pop_back();
			m_ipInputState.cursorPos--;
		}
		
		// 接続ボタン
		const s3d::RoundRect connectButton{ Arg::center(400, 320), 200, 50, 8 };
		bool connectHover = connectButton.mouseOver();
		
		if (connectHover)
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		
		// Enterキーでも接続
		if (connectButton.leftClicked() || KeyEnter.down())
		{
			// 入力されたIPアドレスをパース
			String ipText = m_ipInputState.text;
			Console << U"[クライアント] 接続試行: " << ipText;
			
			// IPアドレスをパース（xxx.xxx.xxx.xxx形式）
			Array<String> parts = ipText.split(U'.');
			if (parts.size() == 4)
			{
				try
				{
					uint8 a = Parse<uint8>(parts[0]);
					uint8 b = Parse<uint8>(parts[1]);
					uint8 c = Parse<uint8>(parts[2]);
					uint8 d = Parse<uint8>(parts[3]);
					
					IPv4Address targetIP{ a, b, c, d };
					
					Console << U"[クライアント] ホストに接続中: " << targetIP.str();
					
					if (m_multiplayer->connect(targetIP, m_gamePort))
					{
						m_isHost = false;
						m_viewMode = ViewMode::Waiting;
						Console << U"[クライアント] 接続開始";
					}
					else
					{
						Console << U"[クライアント] 接続失敗";
					}
				}
				catch (const Error& e)
				{
					Console << U"[クライアント] IPアドレスの解析に失敗: " << ipText;
				}
			}
			else
			{
				Console << U"[クライアント] 無効なIPアドレス形式: " << ipText;
			}
		}
		
		// 戻るボタン
		m_backTr.update(m_backButton.mouseOver());
		if (m_backButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		
		if (m_backButton.leftClicked())
		{
			m_hostDiscovery->stop();
			m_viewMode = ViewMode::Menu;
			m_hostButtons.clear();
			m_hostButtonTransitions.clear();
		}
		
		return;
	}

	// メインメニュー
    m_hostTr.update(m_hostButton.mouseOver());
    m_joinTr.update(m_joinButton.mouseOver());
    m_backTr.update(m_backButton.mouseOver());

    if (m_hostButton.mouseOver() || m_joinButton.mouseOver() || m_backButton.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    if (m_hostButton.leftClicked())
    {
		// 利用可能なポートを自動検索
		auto port = HostDiscovery::findAvailablePort();
		if (!port)
		{
			Console << U"[エラー] 利用可能なポートが見つかりません";
			return;
		}
		
		m_gamePort = *port;
		
		// ローカルIPアドレスを取得して表示
		IPv4Address localIP = detectLocalIPForDisplay();
		m_displayIP = localIP.str();
		
		// ホストとして開始
		m_multiplayer->startHost(m_gamePort);
		m_isHost = true;
		m_viewMode = ViewMode::Waiting;
		
		Console << U"[ホスト] ローカルIP: " << m_displayIP;
		Console << U"[ホスト] ポート" << m_gamePort << U"で接続待機中...";
		Console << U"[ホスト] クライアント側で以下のIPアドレスを入力してください: " << m_displayIP;
    }
    else if (m_joinButton.leftClicked())
    {
		// 手動接続モードに変更
		m_viewMode = ViewMode::HostList;
		
		Console << U"[クライアント] 手動接続モード";
		Console << U"[クライアント] ホストのIPアドレスを入力してください";
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

		if (m_viewMode == ViewMode::Waiting)
		{
			// 接続待機中
			FontAsset(U"TitleFont")(m_isHost ? U"接続待機中..." : U"接続中...")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 72, Vec2{ 400, 150 });
			
			const Font& bold = FontAsset(U"Bold");
			
			if (m_isHost)
			{
				bold(U"相手の接続を待っています...").drawAt(28, Vec2{ 400, 240 }, ColorF{ 0.8 });
				bold(U"クライアント側で以下のIPアドレスを入力してください:").drawAt(22, Vec2{ 400, 290 }, ColorF{ 0.7 });
				
				// IPアドレスを大きく表示
				FontAsset(U"TitleFont")(m_displayIP)
					.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.2, 0.6, 1.0 }, Vec2{ 2, 2 }, ColorF{ 0.0, 0.5 }), 48, Vec2{ 400, 350 });
				
				bold(U"ポート: " + Format(m_gamePort)).drawAt(24, Vec2{ 400, 400 }, ColorF{ 0.6 });
			}
			else
			{
				bold(U"ホストに接続しています...").drawAt(28, Vec2{ 400, 280 }, ColorF{ 0.8 });
			}
			
			m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);
			bold(U"キャンセル").drawAt(28, m_backButton.center(), ColorF{ 0.1 });
		}
		else if (m_viewMode == ViewMode::HostList)
		{
			// IP入力画面
			FontAsset(U"TitleFont")(U"手動接続")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 60, Vec2{ 400, 100 });
			
			const Font& bold = FontAsset(U"Bold");
			
			bold(U"ホストのIPアドレスを入力してください:").drawAt(24, Vec2{ 400, 180 }, ColorF{ 0.8 });
			bold(U"(例: 192.168.1.100)").drawAt(20, Vec2{ 400, 210 }, ColorF{ 0.5 });
			
			// IP入力ボックスを目立つように描画
			const s3d::RoundRect inputBox{ Arg::center(400, 260), 400, 50, 8 };
			inputBox.draw(ColorF{ 0.1, 0.1, 0.15 }).drawFrame(3, ColorF{ 0.3, 0.6, 1.0 });
			
			// 入力中のテキストを表示
			const String displayText = m_ipInputState.text.isEmpty() ? U"IPアドレスを入力..." : m_ipInputState.text;
			const ColorF textColor = m_ipInputState.text.isEmpty() ? ColorF{ 0.4 } : ColorF{ 1.0 };
			bold(displayText).drawAt(28, inputBox.center(), textColor);
			
			// カーソルを点滅表示
			if (m_ipInputState.active && static_cast<int32>(Scene::Time() * 2) % 2 == 0)
			{
				const double textWidth = bold(m_ipInputState.text.substr(0, m_ipInputState.cursorPos)).region(28).w;
				const double cursorX = inputBox.center().x - bold(m_ipInputState.text).region(28).w / 2 + textWidth;
				Line{ cursorX, inputBox.center().y - 14, cursorX, inputBox.center().y + 14 }.draw(2, ColorF{ 1.0 });
			}
			
			// 接続ボタン
			const s3d::RoundRect connectButton{ Arg::center(400, 340), 200, 50, 8 };
			bool connectHover = connectButton.mouseOver();
			connectButton.draw(ColorF{ connectHover ? 0.7 : 0.5 }).drawFrame(2, ColorF{ connectHover ? 1.0 : 0.7 });
			bold(U"接続").drawAt(28, connectButton.center(), ColorF{ 0.1 });
			
			m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);
			bold(U"戻る").drawAt(28, m_backButton.center(), ColorF{ 0.1 });
		}
		else
		{
			// メインメニュー
			FontAsset(U"TitleFont")(U"オンライン対戦")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 72, Vec2{ 400, 150 });

			m_hostButton.draw(ColorF{ 1.0, m_hostTr.value() }).drawFrame(2);
			m_joinButton.draw(ColorF{ 1.0, m_joinTr.value() }).drawFrame(2);
			m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);

			const Font& bold = FontAsset(U"Bold");
			bold(U"ホストとして開始").drawAt(28, m_hostButton.center(), ColorF{ 0.1 });
			bold(U"手動で接続").drawAt(28, m_joinButton.center(), ColorF{ 0.1 });
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
        const s3d::RoundRect panel{ Arg::center(PauseTheme::PanelCenter), PauseTheme::PanelSize, PauseTheme::PanelR };
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


