# pragma once
# include "../Common.hpp"
#include "../ui/PauseMenu.hpp"
#include "../ui/PauseTheme.hpp"

// リザルトシーン
class ResultScene : public App::Scene
{
public:

	ResultScene(const InitData& init);

	void update() override;

	void draw() const override;

private:
	Texture m_titleFrame{ U"assets/ui/frames/result_title_frame.png" };
	Texture m_bigFrame{ U"assets/ui/frames/result_big_frame.png" };

	Texture m_player_win{ U"assets/ui/characters/player_win.png" };
	Texture m_player_loose{ U"assets/ui/characters/player_loose.png" };

	RoundRect m_rematchRect{ Arg::center(200, 240), 300, 100, 4 }; // コンストラクタで初期化
	RoundRect m_lobbyButton{ Arg::center(200, 360), 300, 100, 4 };
	RoundRect m_pauseButton{ Arg::center(200, 480), 300, 100, 4 };

	Transition m_rematchTr{ 0.4s, 0.2s };
	Transition m_lobbyTr{ 0.4s, 0.2s };
	Transition m_pauseTr{ 0.4s, 0.2s };

	// ---- ポーズ用 ----
	void updatePausedUI();
	PauseMenu m_pauseMenu;
	bool m_paused = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

	RoundRect m_resumeButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[1]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_titleButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[2]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };
	RoundRect m_exitPauseButton{ Arg::center(PauseTheme::ButtonXs, PauseTheme::ButtonYs[3]), PauseTheme::ButtonSize.x, PauseTheme::ButtonSize.y, PauseTheme::ButtonR };

	Transition m_resumeTr{ 0.3s, 0.15s };
	Transition m_titleTr{ 0.3s, 0.15s };
	Transition m_exitPauseTr{ 0.3s, 0.15s };

	void updateRenderTargets() const;
	void drawMainContent() const;
	void updatePauseMenu();
	void drawPauseMenu() const;
};


