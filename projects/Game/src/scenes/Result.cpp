# include "Result.hpp"

ResultScene::ResultScene(const InitData& init)
	: IScene{ init }
{

}

void ResultScene::update()
{
	m_rematchTr.update(m_rematchButton.mouseOver());
	m_lobbyTr.update(m_lobbyButton.mouseOver());

	if (m_rematchButton.mouseOver() || m_lobbyButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	if (KeyEscape.down())
	{
		getData().previousState = State::Result;
		changeScene(State::PauseOverlay);
		return;
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
	Scene::SetBackground(ColorF{ 0.15, 0.15, 0.25 });

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


