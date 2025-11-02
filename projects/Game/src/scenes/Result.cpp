# include "Result.hpp"

ResultScene::ResultScene(const InitData& init)
	: IScene{ init }
{

}

void ResultScene::updatePausedUI()
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
		updatePausedUI();
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
		m_pauseMenu.draw();
    }
    else
    {
        m_sceneRT.draw();
    }
}


