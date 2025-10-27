# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../network/MultiplayerManager.hpp"

// 待機（マッチング）シーン
class Matching : public App::Scene
{
public:

	Matching(const InitData& init);

	void update() override;

	void draw() const override;

private:

	// ===== オンライン対戦用 =====
	std::shared_ptr<MultiplayerManager> m_multiplayer;
	TextEditState m_ipInput;
	RoundRect m_hostButton{ Arg::center(400, 240), 300, 60, 8 };
	RoundRect m_joinButton{ Arg::center(400, 320), 300, 60, 8 };
	Transition m_hostTr{ 0.4s, 0.2s };
	Transition m_joinTr{ 0.4s, 0.2s };

	enum class MatchState { SelectMode, Connecting, Connected };
	MatchState m_matchState = MatchState::SelectMode;
	String m_statusMessage;

	// ===== 既存のボタン（ローカルモック用） =====
	RoundRect m_startButton{ Arg::center(400, 400), 300, 60, 8 };
	RoundRect m_backButton{ Arg::center(400, 480), 300, 60, 8 };

	Transition m_startTr{ 0.4s, 0.2s };
	Transition m_backTr{ 0.4s, 0.2s };

	// ---- ポーズ用 ----
	bool m_paused = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

	RoundRect m_resumeButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[0]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_settingsButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[1]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_howToButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[2]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_effectButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[3]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_titleButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[4]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_exitPauseButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[5]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };

	Transition m_resumeTr{ 0.3s, 0.15s };
	Transition m_settingsTr{ 0.3s, 0.15s };
	Transition m_howToTr{ 0.3s, 0.15s };
	Transition m_effectTr{ 0.3s, 0.15s };
	Transition m_titleTr{ 0.3s, 0.15s };
	Transition m_exitPauseTr{ 0.3s, 0.15s };
};


