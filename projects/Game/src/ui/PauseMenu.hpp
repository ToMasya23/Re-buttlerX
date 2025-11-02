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
		Lobby,
		Title,
		Exit,
	};

	void setActions(const Array<Action>& actions)
	{
		m_actions = actions;
		m_tr.assign(actions.size(), Transition{0.3s, 0.15s});
		rebuildLayout();
	}

	// ホバー・クリック更新。必要ならカーソル形状も要求する
	Action update()
	{
		if (m_actions.isEmpty()) 
		{
			return Action::None;
		}

		for (size_t i = 0; i < m_actions.size(); ++i) 
		{
			const bool over = m_buttons[i].mouseOver();
			m_tr[i].update(over);
			if (over) 
			{
				Cursor::RequestStyle(CursorStyle::Hand);
			}

			if (m_buttons[i].leftClicked()) {
				return m_actions[i];
			}
		}
		return Action::None;
	}

	// パネルとボタン描画（背景のブラーや暗転は呼び出し側で実施）
	void draw() const
	{
		if (m_actions.isEmpty()) 
		{
			return;
		}

		const Font& title = FontAsset(U"TitleFont");
		const Font& bold = FontAsset(U"Bold");

		const RoundRect panel{ Arg::center(PauseTheme::PanelCenter), PauseTheme::PanelSize, PauseTheme::PanelR };
		panel.draw(PauseTheme::PanelFill).drawFrame(3, 0, PauseTheme::PanelFrame);

		title(U"PAUSE").drawAt(64, Vec2{ PauseTheme::TitlePos }, PauseTheme::TitleColor);

		for (size_t i = 0; i < m_actions.size(); ++i)
		{
			const auto& rr = m_buttons[i];
			const double a = m_tr[i].value();
			rr.draw(ColorF{ 1.0, a}).drawFrame(2);
			bold(getActionLabel(m_actions[i])).drawAt(28, rr.center().movedBy(0, 0), ColorF{ 0.1 });
		}
	}

private:
	inline String getActionLabel(Action action) const
	{
		switch(action)
		{
			case Action::Resume: return U"再開";
			case Action::Settings: return U"設定";
			case Action::HowToPlay: return U"ゲーム説明";
			case Action::EffectViewer: return U"効果確認";
			case Action::Lobby: return U"ロビーへ";
			case Action::Title: return U"タイトルへ";
			case Action::Exit: return U"ゲーム終了";
		}
	}



	void rebuildLayout()
	{
		constexpr double TitlePadTop = 96.0;
		constexpr double ButtonPadBot = 24.0;
		constexpr double ButtonGapY = 12.0;

		const size_t n = m_actions.size();
		const double buttonsTotalH = (n == 0) ? 0.0 : (n * PauseTheme::ButtonSize.y + (n - 1) * ButtonGapY);
		double panelX = PauseTheme::PanelCenter.x - PauseTheme::PanelSize.x * 0.5;
		double panelY = PauseTheme::PanelCenter.y - (TitlePadTop + buttonsTotalH + ButtonPadBot) * 0.5;

		m_buttons.clear();
		m_buttons.reserve(n);

		double y = panelY + TitlePadTop;
		const double btnX = panelX + PauseTheme::PanelSize.x * 0.5 - PauseTheme::ButtonSize.x * 0.5;
		for (size_t i = 0; i < n; ++i)
		{
			const RoundRect rr{ btnX, y, PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
			m_buttons << rr;
			y += PauseTheme::ButtonSize.y + ButtonGapY;
		}
	}

	Array<Action> m_actions;
	Array<RoundRect> m_buttons;
	Array<Transition> m_tr;
};


