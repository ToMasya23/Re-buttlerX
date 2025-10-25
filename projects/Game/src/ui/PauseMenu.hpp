# pragma once
# include "../Common.hpp"
# include "PauseTheme.hpp"

// ポーズメニュー（任意のシーンに組み込めるヘッダーオンリー部品）
class PauseMenu
{
public:

	enum class Action
	{
		None,
		Resume,
		Settings,
		HowToPlay,
		EffectViewer,
		Title,
		Exit,
	};

	// ホバー・クリック更新。必要ならカーソル形状も要求する
	Action update()
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

		if (m_resumeButton.leftClicked())
		{
			return Action::Resume;
		}
		else if (m_settingsButton.leftClicked())
		{
			return Action::Settings;
		}
		else if (m_howToButton.leftClicked())
		{
			return Action::HowToPlay;
		}
		else if (m_effectButton.leftClicked())
		{
			return Action::EffectViewer;
		}
		else if (m_titleButton.leftClicked())
		{
			return Action::Title;
		}
		else if (m_exitButton.leftClicked())
		{
			return Action::Exit;
		}

		return Action::None;
	}

	// パネルとボタン描画（背景のブラーや暗転は呼び出し側で実施）
	void draw() const
	{
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
	}

private:

	RoundRect m_resumeButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[0]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_settingsButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[1]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_howToButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[2]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_effectButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[3]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_titleButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[4]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_exitButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[5]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };

	Transition m_resumeTr{ 0.3s, 0.15s };
	Transition m_settingsTr{ 0.3s, 0.15s };
	Transition m_howToTr{ 0.3s, 0.15s };
	Transition m_effectTr{ 0.3s, 0.15s };
	Transition m_titleTr{ 0.3s, 0.15s };
	Transition m_exitTr{ 0.3s, 0.15s };
};


