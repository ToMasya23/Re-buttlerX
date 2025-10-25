# include "Game.hpp"

Game::Game(const InitData& init)
	: IScene{ init }
{
	for (int32 y = 0; y < 5; ++y)
	{
		for (int32 x = 0; x < (800 / BrickSize.x); ++x)
		{
			m_bricks << Rect{ (x * BrickSize.x), (60 + y * BrickSize.y), BrickSize };
		}
	}
}

void Game::update()
{
    // Esc でポーズをトグル
    if (KeyEscape.down())
    {
        m_paused = (not m_paused);
        return;
    }

    if (m_paused)
    {
        // ポーズ中はメニューのホバー/クリックのみ処理
        m_resumeTr.update(m_resumeButton.mouseOver());
        m_settingsTr.update(m_settingsButton.mouseOver());
        m_howToTr.update(m_howToButton.mouseOver());
        m_effectTr.update(m_effectButton.mouseOver());
        m_titleTr.update(m_titleButton.mouseOver());
        m_exitTr.update(m_exitButton.mouseOver());

        if (m_resumeButton.mouseOver() || m_settingsButton.mouseOver() || m_howToButton.mouseOver()
            || m_effectButton.mouseOver() || m_titleButton.mouseOver() || m_exitButton.mouseOver())
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
        else if (m_exitButton.leftClicked())
        {
            System::Exit();
        }

        return;
    }

    // --- 通常のゲーム更新 ---
    // ボールを移動させる
    m_ball.moveBy(m_ballVelocity * Scene::DeltaTime());

    // ブロックを順にチェックする
    for (auto it = m_bricks.begin(); it != m_bricks.end(); ++it)
    {
        // ブロックとボールが交差していたら
        if (it->intersects(m_ball))
        {
            // ブロックの上辺、または底辺と交差していたら
            if (it->bottom().intersects(m_ball) || it->top().intersects(m_ball))
            {
                m_ballVelocity.y *= -1;
            }
            else // ブロックの左辺または右辺と交差していたら
            {
                m_ballVelocity.x *= -1;
            }

            // ブロックを配列から削除する（イテレータは無効になる）
            m_bricks.erase(it);

            m_brickSound.playOneShot(0.5);

            ++m_score;

            break;
        }
    }

    // 天井にぶつかったら
    if ((m_ball.y < 0) && (m_ballVelocity.y < 0))
    {
        m_ballVelocity.y *= -1;
    }

    // 左右の壁にぶつかったら
    if (((m_ball.x < 0) && (m_ballVelocity.x < 0))
        || ((800 < m_ball.x) && (0 < m_ballVelocity.x)))
    {
        m_ballVelocity.x *= -1;
    }

    // パドルにあたったらはね返る
    if (const Rect paddle = getPaddle();
        (0 < m_ballVelocity.y) && paddle.intersects(m_ball))
    {
        // パドルの中心からの距離に応じてはね返る方向を変える
        m_ballVelocity = Vec2{ (m_ball.x - paddle.center().x) * 10, -m_ballVelocity.y }.setLength(BallSpeed);
    }

    // 画面外に出るか、ブロックが無くなったら
    if ((600 < m_ball.y) || m_bricks.isEmpty())
    {
        // リザルトへ
        getData().lastScore = m_score;
        changeScene(State::Result);
    }
}

void Game::draw() const
{
    const Size sceneSize = Scene::Size();

    // レンダーターゲットの用意（サイズが変わっていたら作り直す）
    if ((not m_sceneRT) || (m_sceneRT.size() != sceneSize))
    {
        const_cast<Game*>(this)->m_sceneRT = RenderTexture{ sceneSize };
    }
    if ((not m_blurInternal) || (m_blurInternal.size() != sceneSize))
    {
        const_cast<Game*>(this)->m_blurInternal = RenderTexture{ sceneSize };
    }
    if ((not m_blurTarget) || (m_blurTarget.size() != sceneSize))
    {
        const_cast<Game*>(this)->m_blurTarget = RenderTexture{ sceneSize };
    }

    // ゲーム画面をオフスクリーンに描画
    {
        const ScopedRenderTarget2D rt{ m_sceneRT }; 
        m_sceneRT.clear(ColorF{ 0.2 });

        // すべてのブロックを描画する
        for (const auto& brick : m_bricks)
        {
            brick.stretched(-1).draw(HSV{ brick.y - 40 });
        }

        // ボールを描く
        m_ball.draw();

        // パドルを描く
        getPaddle().rounded(3).draw();

        // スコアを描く
        FontAsset(U"Bold")(m_score).draw(24, Vec2{ 400, 16 });
    }

    // 画面へ転送（ポーズ時はブラー＋オーバーレイ）
    if (m_paused)
    {
        Shader::GaussianBlur(m_sceneRT, m_blurInternal, m_blurTarget, BoxFilterSize::BoxFilter9x9);
        m_blurTarget.draw();

        // 暗転オーバーレイ
        Rect{ sceneSize }.draw(PauseTheme::Dimmer);

        // メニューパネル・ボタン
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
        m_exitButton.draw(ColorF{ 1.0, m_exitTr.value() }).drawFrame(2);

        bold(U"再開").drawAt(28, m_resumeButton.center(), ColorF{ 0.1 });
        bold(U"設定").drawAt(28, m_settingsButton.center(), ColorF{ 0.1 });
        bold(U"ゲーム説明").drawAt(28, m_howToButton.center(), ColorF{ 0.1 });
        bold(U"効果確認").drawAt(28, m_effectButton.center(), ColorF{ 0.1 });
        bold(U"タイトルへ").drawAt(28, m_titleButton.center(), ColorF{ 0.1 });
        bold(U"EXIT").drawAt(28, m_exitButton.center(), ColorF{ 0.1 });

        // ポーズ中はマウスカーソル表示
        Cursor::RequestStyle(CursorStyle::Default);
    }
    else
    {
        m_sceneRT.draw();

        // プレイ中はマウスカーソルを非表示
        Cursor::RequestStyle(CursorStyle::Hidden);
    }
}

Rect Game::getPaddle() const
{
	return{ Arg::center(Cursor::Pos().x, 500), 60, 10 };
}


