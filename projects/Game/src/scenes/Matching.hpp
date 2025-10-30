# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../network/MultiplayerManager.hpp"
# include "../network/HostDiscovery.hpp"

// 待機（マッチング）シーン
class Matching : public App::Scene
{
public:

	Matching(const InitData& init);

	void update() override;

	void draw() const override;

private:

	enum class ViewMode
	{
		Menu,          // メインメニュー
		HostList,      // ホストリスト表示
		Waiting        // 接続待機中
	};

	ViewMode m_viewMode = ViewMode::Menu;

	RoundRect m_hostButton{ Arg::center(400, 280), 300, 60, 8 };
	RoundRect m_joinButton{ Arg::center(400, 360), 300, 60, 8 };
	RoundRect m_backButton{ Arg::center(400, 440), 300, 60, 8 };

	Transition m_hostTr{ 0.4s, 0.2s };
	Transition m_joinTr{ 0.4s, 0.2s };
	Transition m_backTr{ 0.4s, 0.2s };
	
	// ネットワーク関連
	std::shared_ptr<MultiplayerManager> m_multiplayer;
	std::unique_ptr<HostDiscovery> m_hostDiscovery;
	bool m_isHost = false;
	uint16 m_gamePort = 12345;
	
	// ホストリスト用
	Array<RoundRect> m_hostButtons;
	Array<Transition> m_hostButtonTransitions;
	int32 m_selectedHostIndex = -1;

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


