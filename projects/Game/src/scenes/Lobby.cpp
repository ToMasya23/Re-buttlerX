# include "Lobby.hpp"

Lobby::Lobby(const InitData& init)
	: IScene{ init }
{

}

void Lobby::update()
{
	m_pvpTr.update(m_pvpButton.mouseOver());
	m_pveTr.update(m_pveButton.mouseOver());
	m_exitTr.update(m_exitButton.mouseOver());

	if (m_pvpButton.mouseOver() || m_pveButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	if (KeyEscape.down())
	{
		getData().previousState = State::Lobby;
		changeScene(State::PauseOverlay);
		return;
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
	}
}

void Lobby::draw() const
{
	Scene::SetBackground(ColorF{ 0.25, 0.25, 0.35 });

	FontAsset(U"TitleFont")(U"ロビー")
		.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.15, 0.2 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 100, Vec2{ 400, 140 });

	m_pvpButton.draw(ColorF{ 1.0, m_pvpTr.value() }).drawFrame(2);
	m_pveButton.draw(ColorF{ 1.0, m_pveTr.value() }).drawFrame(2);

	const Font& bold = FontAsset(U"Bold");
	bold(U"PvP: マッチングへ").drawAt(28, m_pvpButton.center(), ColorF{ 0.1 });
	bold(U"PvE: すぐ開始").drawAt(28, m_pveButton.center(), ColorF{ 0.1 });
}


