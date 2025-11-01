# include "Result.hpp"

ResultScene::ResultScene(const InitData& init)
	: IScene{ init }
{

}

void ResultScene::update()
{
    // ポーズ切り替え
    if (KeyEscape.down())
    {
        m_paused = not m_paused;
        return;
    }

    // ポーズ中の更新
    if (m_paused)
    {
        updatePauseMenu();
        return;
    }

    // 通常の更新
    m_rematchTr.update(m_rematchRect.mouseOver());
    m_lobbyTr.update(m_lobbyButton.mouseOver());
    m_pauseTr.update(m_pauseButton.mouseOver());

    // マウスカーソルの更新
    if (m_rematchRect.mouseOver() or m_lobbyButton.mouseOver() or m_pauseButton.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    // ボタンクリック処理
    if (m_rematchRect.leftClicked())
    {
        changeScene(getData().lastMode == GameData::GameMode::PvP ? State::Matching : State::Game);
    }
    else if (m_lobbyButton.leftClicked())
    {
        changeScene(State::Lobby);
    }
    else if (m_pauseButton.leftClicked())
    {
        m_paused = true;
    }
}

void ResultScene::draw() const
{
    updateRenderTargets();
    
    {
        const ScopedRenderTarget2D rt{ m_sceneRT };
        // 単色背景を削除（グラデーション背景はdrawMainContent内で描画）
        
        drawMainContent();
    }

    if (m_paused)
    {
        drawPauseMenu();
    }
    else
    {
        m_sceneRT.draw();
    }
}

void ResultScene::updateRenderTargets() const
{
    const Size sceneSize = Scene::Size();
    auto* scene = const_cast<ResultScene*>(this);

    if ((not m_sceneRT) or (m_sceneRT.size() != sceneSize))
        scene->m_sceneRT = RenderTexture{ sceneSize };
    if ((not m_blurInternal) or (m_blurInternal.size() != sceneSize))
        scene->m_blurInternal = RenderTexture{ sceneSize };
    if ((not m_blurTarget) or (m_blurTarget.size() != sceneSize))
        scene->m_blurTarget = RenderTexture{ sceneSize };
}

void ResultScene::drawMainContent() const
{
    const Vec2 resultPos{ 400, 90 };
    const Font& title = FontAsset(U"TitleFont");
    const Font& bold = FontAsset(U"Bold");

    // 画面サイズ取得
    const Size sceneSize = Scene::Size();

	if (getData().lastScore)
	{
		const ColorF topColor{ 0.165, 0.102, 0.353 };    // #2a1a5a
		const ColorF bottomColor{ 0.988, 0.278, 0.792 }; // #fc47ca
    
		// 上部200pxは単色
		Rect{ 0, 0, sceneSize.x, 200 }.draw(topColor);
    
		// 200px以降から最下部までグラデーション
		Rect{ 0, 200, sceneSize.x, sceneSize.y - 200 }
			.draw(Arg::top = topColor, Arg::bottom = bottomColor);

		// タイトルフレーム描画
		m_titleFrame.resized(720, 370).drawAt(Vec2{ 400, 225 });

		// 大きな背景フレーム
		m_bigFrame.resized(800, 970).drawAt(Vec2{ 400, 482 });		

		// リマッチボタンの描画
		m_rematchRect.draw(ColorF{ 0.996, 0.420, 0.592, m_rematchTr.value() }).drawFrame(10, ColorF{ 0.996, 0.420, 0.592 });

		// ロビーボタンの描画
		m_lobbyButton.draw(ColorF{ 0.996, 0.420, 0.592, m_lobbyTr.value() }).drawFrame(10, ColorF{ 0.996, 0.420, 0.592 });

		// ポーズボタンの描画
		m_pauseButton.draw(ColorF{ 0.996, 0.420, 0.592, m_pauseTr.value() }).drawFrame(10, ColorF{ 0.996, 0.420, 0.592 });

    // 勝敗テキストと画像
        title(U"はい論破〜☆ 次のかまちょどうぞ♡")
            .drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.902, 0.208, 0.765 }, Vec2{ 3, 3 }, 
                ColorF{ 0.996, 0.694, 0.906 }), 40, resultPos);
        m_player_win.resized(350, 350).drawAt(Vec2{ 550, 350 });
    }
    else
    {
		const ColorF topColor{ 0.165, 0.102, 0.353 };    // #2a1a5a
		const ColorF bottomColor{ 0.471, 0.0, 1.0 }; // #7800ff

		// 上部200pxは単色
		Rect{ 0, 0, sceneSize.x, 200 }.draw(topColor);

		// 200px以降から最下部までグラデーション
		Rect{ 0, 200, sceneSize.x, sceneSize.y - 200 }
		.draw(Arg::top = topColor, Arg::bottom = bottomColor);

		// タイトルフレーム描画
		m_titleFrame.resized(720, 370).drawAt(Vec2{ 400, 225 });

		// 大きな背景フレーム
		m_bigFrame.resized(800, 970).drawAt(Vec2{ 400, 482 });

		// リマッチボタンの描画
		m_rematchRect.draw(ColorF{ 0.996, 0.420, 0.592, m_rematchTr.value() }).drawFrame(10, ColorF{ 0.996, 0.420, 0.592 });

		// ロビーボタンの描画
		m_lobbyButton.draw(ColorF{ 0.996, 0.420, 0.592, m_lobbyTr.value() }).drawFrame(10, ColorF{ 0.996, 0.420, 0.592 });

		// ポーズボタンの描画
		m_pauseButton.draw(ColorF{ 0.996, 0.420, 0.592, m_pauseTr.value() }).drawFrame(10, ColorF{ 0.996, 0.420, 0.592 });
        title(U"もう寝る！！起きたら勝ってる予定！")
            .drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.0, 0.216, 0.996 }, Vec2{ 3, 3 },
                ColorF{ 0.471, 0.0, 1.0 }), 40, resultPos);
        m_player_loose.resized(350, 350).drawAt(Vec2{ 550, 350 });
    }

    // ボタンテキスト
    bold(U"再戦").drawAt(28, m_rematchRect.center(), ColorF{ 1 });
    bold(U"ロビーへ").drawAt(28, m_lobbyButton.center(), ColorF{ 1 });
    bold(U"ポーズ").drawAt(28, m_pauseButton.center(), ColorF{ 1 });
}

void ResultScene::updatePauseMenu()
{
	m_resumeTr.update(m_resumeButton.mouseOver());
    m_titleTr.update(m_titleButton.mouseOver());
    m_exitPauseTr.update(m_exitPauseButton.mouseOver());

    if (m_resumeButton.mouseOver() or m_titleButton.mouseOver() or m_exitPauseButton.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }
	if (m_resumeButton.leftClicked())
	{
		m_paused = false;
	}
	else if (m_titleButton.leftClicked())
        changeScene(State::Title);
    else if (m_exitPauseButton.leftClicked())
        System::Exit();
}

void ResultScene::drawPauseMenu() const
{
    Shader::GaussianBlur(m_sceneRT, m_blurInternal, m_blurTarget, BoxFilterSize::BoxFilter9x9);
    m_blurTarget.draw();
    Rect{ Scene::Size() }.draw(PauseTheme::Dimmer);

    const RoundRect panel{ Arg::center(PauseTheme::PanelCenter), PauseTheme::PanelSize, PauseTheme::PanelR };

    const Font& title = FontAsset(U"TitleFont");
    const Font& bold = FontAsset(U"Bold");

    // ポーズメニューボタンの描画
	m_resumeButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_resumeTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
	m_titleButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_titleTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });
	m_exitPauseButton.draw(ColorF{ 0.925f, 0.714f, 0.882f, m_exitPauseTr.value() }).drawFrame(4, ColorF{ 0.925f, 0.714f, 0.882f });


    // ボタンテキストの描画
    bold(U"再開").drawAt(28, m_resumeButton.center(), ColorF{ 1 });
    bold(U"タイトルへ").drawAt(28, m_titleButton.center(), ColorF{ 1 });
    bold(U"EXIT").drawAt(28, m_exitPauseButton.center(), ColorF{ 1 });
}


