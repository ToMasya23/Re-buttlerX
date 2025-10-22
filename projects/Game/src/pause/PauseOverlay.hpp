# pragma once
# include "../Common.hpp"

// ポーズメニュー（フローティングウィンドウ的に扱うがシーンとして実装）
class PauseOverlay : public App::Scene
{
public:

	PauseOverlay(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_resumeButton{ Arg::center(400, 280), 320, 56, 8 };
	RoundRect m_settingsButton{ Arg::center(400, 350), 320, 56, 8 };
	RoundRect m_howToButton{ Arg::center(400, 420), 320, 56, 8 };
	RoundRect m_effectButton{ Arg::center(400, 490), 320, 56, 8 };
	RoundRect m_titleButton{ Arg::center(400, 560), 320, 56, 8 };
	RoundRect m_exitButton{ Arg::center(400, 630), 320, 56, 8 };

	Transition m_resumeTr{ 0.3s, 0.15s };
	Transition m_settingsTr{ 0.3s, 0.15s };
	Transition m_howToTr{ 0.3s, 0.15s };
	Transition m_effectTr{ 0.3s, 0.15s };
	Transition m_titleTr{ 0.3s, 0.15s };
	Transition m_exitTr{ 0.3s, 0.15s };
};


