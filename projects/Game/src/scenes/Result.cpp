# include "Result.hpp"

ResultScene::ResultScene(const InitData& init)
	: IScene{ init }
{

}

void ResultScene::update()
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

    m_rematchTr.update(m_rematchButton.mouseOver());
    m_lobbyTr.update(m_lobbyButton.mouseOver());

    if (m_rematchButton.mouseOver() || m_lobbyButton.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    if (m_rematchButton.leftClicked())
    {
        if (getData().lastMode == GameData::GameMode::PvP)
        {
            changeScene(State::Matching);
        }
        else
        {
            changeScene(State::Game);
        }
    }
    else if (m_lobbyButton.leftClicked())
    {
        changeScene(State::Lobby);
    }
}

void ResultScene::draw() const
{
    const Size sceneSize = Scene::Size();

    if ((not m_sceneRT) || (m_sceneRT.size() != sceneSize))
    {
        const_cast<ResultScene*>(this)->m_sceneRT = RenderTexture{ sceneSize };
    }
    if ((not m_blurInternal) || (m_blurInternal.size() != sceneSize))
    {
        const_cast<ResultScene*>(this)->m_blurInternal = RenderTexture{ sceneSize };
    }
    if ((not m_blurTarget) || (m_blurTarget.size() != sceneSize))
    {
        const_cast<ResultScene*>(this)->m_blurTarget = RenderTexture{ sceneSize };
    }

    {
        const ScopedRenderTarget2D rt{ m_sceneRT };
        m_sceneRT.clear(ColorF{ 0.15, 0.15, 0.25 });

        const Font& title = FontAsset(U"TitleFont");
        const Font& bold = FontAsset(U"Bold");
        const String modeStr = (getData().lastMode == GameData::GameMode::PvP) ? U"PvP" : ((getData().lastMode == GameData::GameMode::PvE) ? U"PvE" : U"Unknown");

        title(U"リザルト").drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.2 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 96, Vec2{ 400, 140 });
        bold(U"Score: {}"_fmt(getData().lastScore)).drawAt(28, Vec2{ 400, 240 }, ColorF{ 0.9 });
        bold(U"Mode: {}"_fmt(modeStr)).drawAt(24, Vec2{ 400, 280 }, ColorF{ 0.9 });

        m_rematchButton.draw(ColorF{ 1.0, m_rematchTr.value() }).drawFrame(2);
        m_lobbyButton.draw(ColorF{ 1.0, m_lobbyTr.value() }).drawFrame(2);

        bold(U"再戦").drawAt(28, m_rematchButton.center(), ColorF{ 0.1 });
        bold(U"ロビーへ").drawAt(28, m_lobbyButton.center(), ColorF{ 0.1 });
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


