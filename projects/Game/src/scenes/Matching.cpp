# include "Matching.hpp"

Matching::Matching(const InitData& init)
	: IScene{ init }
{

}

void Matching::update()
{
	m_startTr.update(m_startButton.mouseOver());
	m_backTr.update(m_backButton.mouseOver());

	if (m_startButton.mouseOver() || m_backButton.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	if (KeyEscape.down())
	{
		getData().previousState = State::Matching;
		changeScene(State::PauseOverlay);
		return;
	}

	if (m_startButton.leftClicked())
	{
		getData().lastMode = GameData::GameMode::PvP;
		changeScene(State::Game);
	}
	else if (m_backButton.leftClicked())
	{
		changeScene(State::Lobby);
	}
}

void Matching::draw() const
{
	Scene::SetBackground(ColorF{ 0.2, 0.2, 0.2 });

	FontAsset(U"TitleFont")(U"待機中...")
		.drawAt(TextStyle::OutlineShadow(0.2, ColorF{ 0.1, 0.1, 0.1 }, Vec2{ 3, 3 }, ColorF{ 0.0, 0.5 }), 72, Vec2{ 400, 200 });

	m_startButton.draw(ColorF{ 1.0, m_startTr.value() }).drawFrame(2);
	m_backButton.draw(ColorF{ 1.0, m_backTr.value() }).drawFrame(2);

	const Font& bold = FontAsset(U"Bold");
	bold(U"開始（モック）").drawAt(28, m_startButton.center(), ColorF{ 0.1 });
	bold(U"ロビーに戻る").drawAt(28, m_backButton.center(), ColorF{ 0.1 });
}


