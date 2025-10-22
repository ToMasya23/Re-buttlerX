# include "PauseOverlay.hpp"

PauseOverlay::PauseOverlay(const InitData& init)
	: IScene{ init }
{

}

void PauseOverlay::update()
{
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

	if (KeyEscape.down())
	{
		// 直前のシーンに戻る
		changeScene(getData().previousState);
		return;
	}

	if (m_resumeButton.leftClicked())
	{
		changeScene(getData().previousState);
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
}

void PauseOverlay::draw() const
{
	// 背景を半透明で暗くする
	Scene::Rect().draw(ColorF{ 0.0, 0.6 });

	const Font& title = FontAsset(U"TitleFont");
	const Font& bold = FontAsset(U"Bold");

	// 中央パネル
	const RoundRect panel{ Arg::center(400, 430), 520, 460, 12 };
	panel.draw(ColorF{ 0.95, 0.95, 0.96 }).drawFrame(3, 0, ColorF{ 0.2, 0.2, 0.3 });

	title(U"PAUSE").drawAt(64, Vec2{ 400, 230 }, ColorF{ 0.15 });

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
}


