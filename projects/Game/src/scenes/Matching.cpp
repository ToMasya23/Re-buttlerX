#include "Matching.hpp"
#include "../network/NetworkPlatform.hpp"

Matching::Matching(const InitData& init)
	: IScene{ init }
{
	m_multiplayer = std::make_shared<MultiplayerManager>();
	m_hostDiscovery = std::make_unique<HostDiscovery>();
	m_udpDiscovery = std::make_unique<UDPDiscovery>();
	
	// IP入力のデフォルト値を設定
	m_ipInputState.text = U"192.168.1.100";
	m_ipInputState.cursorPos = m_ipInputState.text.size();
	
	// 合言葉入力のデフォルト値を設定
	m_passphraseInputState.text = U"";
	m_passphraseInputState.cursorPos = 0;
}

IPv4Address Matching::detectLocalIPForDisplay()
{
	Array<IPv4Address> localIPs = NetworkPlatform::GetLocalIPAddresses();
	if (!localIPs.isEmpty())
	{
		return localIPs[0];
	}
	return IPv4Address{ 127, 0, 0, 1 };
}

void Matching::updatePausedUI()
{
	m_pauseMenu.setActions({
		PauseMenu::Action::Resume,
		PauseMenu::Action::Lobby,
		PauseMenu::Action::Title,
		PauseMenu::Action::Exit,
	});

	const PauseMenu::Action action = m_pauseMenu.update();
	getData().pauseReturnState = State::Lobby;
	switch (action)
	{
	case PauseMenu::Action::Resume:
		m_paused = false;
		break;
	
	case PauseMenu::Action::Lobby:
		changeScene(State::Lobby);
		break;
	case PauseMenu::Action::Title:
		changeScene(State::Title);
		break;
	case PauseMenu::Action::Exit:
		System::Exit();
		break;
	default:
		break;
	}
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
		updatePausedUI();
    }

	// UDP Discovery を更新
	if (m_udpDiscovery)
	{
		m_udpDiscovery->update();
		
		// クライアント側: ホストが見つかったら自動接続
		if (m_udpDiscovery->getRole() == UDPDiscovery::Role::Client)
		{
			const auto& hosts = m_udpDiscovery->getDiscoveredHosts();
			if (!hosts.isEmpty() && m_viewMode == ViewMode::PassphraseInput)
			{
				// 最初に見つかったホストに接続
				const auto& host = hosts[0];
				//Console << U"[クライアント] ホスト発見！自動接続開始: " << host.address.str();
				
				if (m_multiplayer->connect(host.address, host.port))
				{
					m_isHost = false;
					m_viewMode = ViewMode::Waiting;
					m_udpDiscovery->stopSearching();
				}
			}
		}
	}
	
	// ホスト検索を更新（旧方式・フォールバック用）
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
				//Console << U"[ホスト] 接続待機中... isConnected=" << connected;
			}
			else
			{
				//Console << U"[クライアント] 接続確認中... isConnected=" << connected;
			}
			lastCheckTime = Scene::Time();
		}
		
		if (m_multiplayer->isConnected())
		{
			// 接続成功、ゲームシーンへ
			if (m_isHost)
			{
				//Console << U"[ホスト] クライアントとの接続が確立されました。Game画面に遷移します。";
			m_udpDiscovery->stopAdvertising();
			}
			else
			{
				//Console << U"[クライアント] ホストとの接続が確立されました。Game画面に遷移します。";
			m_udpDiscovery->stopSearching();
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
			m_udpDiscovery->stopAdvertising();
			m_udpDiscovery->stopSearching();
			m_viewMode = ViewMode::Menu;
		}
		
		m_backTr.update(m_backButton.mouseOver());
		if (m_backButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		
		return;
	}
	
	// 合言葉入力モード
	if (m_viewMode == ViewMode::PassphraseInput)
	{
		// テキスト入力の処理
		m_passphraseInputState.active = true;
		
		// TextInputからの入力を取得
		const String input = TextInput::GetRawInput();
		
		// 文字を追加（合言葉は日本語も可能）
		m_passphraseInputState.text += input;
		m_passphraseInputState.cursorPos = m_passphraseInputState.text.size();
		
		// バックスペース
		if (KeyBackspace.down() && m_passphraseInputState.cursorPos > 0)
		{
			m_passphraseInputState.text.pop_back();
			m_passphraseInputState.cursorPos--;
		}
		
		// 確定ボタン
		const s3d::RoundRect confirmButton{ Arg::center(400, 340), 200, 50, 8 };
		bool confirmHover = confirmButton.mouseOver();
		
		if (confirmHover)
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}
		
		// Enterキーでも確定
		if (confirmButton.leftClicked() || KeyEnter.down())
		{
			String normalized = m_passphraseInputState.text;
			while (!normalized.isEmpty() && (normalized.back() <= U' '))
			{
				normalized.pop_back();
			}

			if (!normalized.isEmpty())
			{
				m_passphraseInputState.text = normalized;
				m_passphraseInputState.cursorPos = normalized.size();
				m_passphrase = normalized;
				
				if (m_isHost)
				{
					// ホスト: 合言葉で部屋を立てる
					//Console << U"[ホスト] 合言葉設定: " << m_passphrase;
					
					// ローカルIPアドレスを取得
					IPv4Address localIP = detectLocalIPForDisplay();
					m_displayIP = localIP.str();
					
					// TCPサーバー開始
					m_multiplayer->startHost(m_gamePort);
					
					// UDP広告開始
					if (m_udpDiscovery->startAdvertising(m_passphrase, U"Re-ButtlerX", m_gamePort))
					{
						m_viewMode = ViewMode::Waiting;
						//Console << U"[ホスト] 接続待機中... IP: " << m_displayIP;
					}
					else
					{
						//Console << U"[エラー] UDP広告の開始に失敗";
						m_multiplayer->disconnect();
						m_viewMode = ViewMode::Menu;
					}
				}
				else
				{
					// クライアント: 合言葉でブロードキャスト開始
					//Console << U"[クライアント] 合言葉入力: " << m_passphrase;
					
					if (m_udpDiscovery->startSearching(m_passphrase))
					{
						//Console << U"[クライアント] ホスト検索中...";
						// ViewModeはPassphraseInputのまま（自動接続処理が行われる）
					}
					else
					{
						//Console << U"[エラー] UDP検索の開始に失敗";
						m_viewMode = ViewMode::Menu;
					}
				}
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
			m_udpDiscovery->stopAdvertising();
			m_udpDiscovery->stopSearching();
			m_viewMode = ViewMode::Menu;
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
			//Console << U"[クライアント] 接続試行: " << ipText;
			
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
					
					//Console << U"[クライアント] ホストに接続中: " << targetIP.str();
					
					if (m_multiplayer->connect(targetIP, m_gamePort))
					{
						m_isHost = false;
						m_viewMode = ViewMode::Waiting;
						//Console << U"[クライアント] 接続開始";
					}
					else
					{
						//Console << U"[クライアント] 接続失敗";
					}
				}
				catch (const Error& e)
				{
					(void)e;  // 未使用変数（将来のログ出力用に保持）
					//Console << U"[クライアント] IPアドレスの解析に失敗: " << ipText;
				}
			}
			else
			{
				//Console << U"[クライアント] 無効なIPアドレス形式: " << ipText;
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
		// 合言葉入力モードへ移行（ホスト）
		m_isHost = true;
		m_viewMode = ViewMode::PassphraseInput;
		m_passphraseInputState.text = U"";
		m_passphraseInputState.cursorPos = 0;
		
		//Console << U"[ホスト] 合言葉入力画面へ";
    }
    else if (m_joinButton.leftClicked())
    {
		// 合言葉入力モードへ移行（ゲスト）
		m_isHost = false;
		m_viewMode = ViewMode::PassphraseInput;
		m_passphraseInputState.text = U"";
		m_passphraseInputState.cursorPos = 0;
		
		//Console << U"[ゲスト] 合言葉入力画面へ";
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

		if (m_viewMode == ViewMode::PassphraseInput)
		{
			// 合言葉入力画面
			FontAsset(U"TitleFont")(m_isHost ? U"ホスト：合言葉設定" : U"ゲスト：合言葉入力")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 60, Vec2{ 400, 100 });
			
			const Font& bold = FontAsset(U"Bold");
			
			if (m_isHost)
			{
				bold(U"部屋を立てるための合言葉を入力してください").drawAt(24, Vec2{ 400, 180 }, ColorF{ 0.8 });
				bold(U"(ゲスト側も同じ合言葉を入力します)").drawAt(20, Vec2{ 400, 210 }, ColorF{ 0.5 });
				bold(U"[演出] SNSに投稿する...").drawAt(20, Vec2{ 400, 240 }, ColorF{ 0.3, 0.8, 1.0 });
			}
			else
			{
				bold(U"ホストが設定した合言葉を入力してください").drawAt(24, Vec2{ 400, 180 }, ColorF{ 0.8 });
				bold(U"(入力後、自動的にホストを検索します)").drawAt(20, Vec2{ 400, 210 }, ColorF{ 0.5 });
				bold(U"[演出] 噛みつく！").drawAt(20, Vec2{ 400, 240 }, ColorF{ 1.0, 0.3, 0.3 });
			}
			
			// 合言葉入力ボックス
			const s3d::RoundRect inputBox{ Arg::center(400, 290), 400, 50, 8 };
			inputBox.draw(ColorF{ 0.1, 0.1, 0.15 }).drawFrame(3, ColorF{ 0.3, 0.6, 1.0 });
			
			// 入力中のテキストを表示
			const String displayText = m_passphraseInputState.text.isEmpty() ? U"合言葉を入力..." : m_passphraseInputState.text;
			const ColorF textColor = m_passphraseInputState.text.isEmpty() ? ColorF{ 0.4 } : ColorF{ 1.0 };
			bold(displayText).drawAt(28, inputBox.center(), textColor);
			
			// カーソルを点滅表示
			if (m_passphraseInputState.active && static_cast<int32>(Scene::Time() * 2) % 2 == 0)
			{
				const double textWidth = bold(m_passphraseInputState.text.substr(0, m_passphraseInputState.cursorPos)).region(28).w;
				const double cursorX = inputBox.center().x - bold(m_passphraseInputState.text).region(28).w / 2 + textWidth;
				Line{ cursorX, inputBox.center().y - 14, cursorX, inputBox.center().y + 14 }.draw(2, ColorF{ 1.0 });
			}
			
			// 確定ボタン
			const s3d::RoundRect confirmButton{ Arg::center(400, 340), 200, 50, 8 };
			const bool confirmHover = confirmButton.mouseOver();
			const bool canConfirm = !m_passphraseInputState.text.isEmpty();
			confirmButton.draw(canConfirm ? ColorF{ 0.2, 0.6, 1.0, confirmHover ? 1.0 : 0.8 } : ColorF{ 0.3, 0.3, 0.3 })
				.drawFrame(2, ColorF{ 1.0 });
			bold(m_isHost ? U"部屋を立てる" : U"噛みつく！").drawAt(24, confirmButton.center(), canConfirm ? ColorF{ 1.0 } : ColorF{ 0.5 });
			
			m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);
			bold(U"戻る").drawAt(28, m_backButton.center(), ColorF{ 0.1 });
		}
		else if (m_viewMode == ViewMode::Waiting)
		{
			// 接続待機中
			FontAsset(U"TitleFont")(m_isHost ? U"接続待機中..." : U"接続中...")
				.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 72, Vec2{ 400, 150 });
			
			const Font& bold = FontAsset(U"Bold");
			
			if (m_isHost)
			{
				bold(U"[演出] SNSに投稿しました！").drawAt(24, Vec2{ 400, 230 }, ColorF{ 0.3, 0.8, 1.0 });
				bold(U"合言葉: " + m_passphrase).drawAt(28, Vec2{ 400, 270 }, ColorF{ 1.0, 1.0, 0.3 });
				bold(U"相手が噛みつくのを待っています...").drawAt(22, Vec2{ 400, 310 }, ColorF{ 0.7 });
				
				// IPアドレスを表示（フォールバック用）
				bold(U"（手動接続用IP: " + m_displayIP + U"）").drawAt(18, Vec2{ 400, 360 }, ColorF{ 0.5 });
				bold(U"（ポート: " + Format(m_gamePort) + U"）").drawAt(18, Vec2{ 400, 385 }, ColorF{ 0.5 });
			}
			else
			{
				if (m_udpDiscovery && m_udpDiscovery->isActive())
				{
					bold(U"[演出] 噛みつき中...").drawAt(24, Vec2{ 400, 250 }, ColorF{ 1.0, 0.3, 0.3 });
					bold(U"合言葉: " + m_passphrase).drawAt(28, Vec2{ 400, 290 }, ColorF{ 1.0, 1.0, 0.3 });
					bold(U"ホストを探しています...").drawAt(22, Vec2{ 400, 330 }, ColorF{ 0.7 });
				}
				else
				{
					bold(U"ホストに接続しています...").drawAt(28, Vec2{ 400, 280 }, ColorF{ 0.8 });
				}
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
		m_pauseMenu.draw();
	}
    else
    {
        m_sceneRT.draw();
    }
}

