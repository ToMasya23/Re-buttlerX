# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/BattleLayout.hpp"

// ゲームシーン（PvE バトル）
class Game : public App::Scene
{
public:

	Game(const InitData& init);

	void update() override;

	void draw() const override;

private:

	// ---- バトル用 ----
	static constexpr int32 MaxHP = 100;
	int32 m_playerHP = MaxHP;
	int32 m_enemyHP = MaxHP;
	bool m_showAttackOptions = false;

	// ---- ポーズ用 ----
	bool m_paused = false;
	bool m_pauseAwaitingCapture = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

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

	// ボタンのホバー演出
	Transition m_attackTr{ 0.3s, 0.15s };
	Transition m_escapeTr{ 0.3s, 0.15s };
	Transition m_attack1Tr{ 0.3s, 0.15s };
	Transition m_attack2Tr{ 0.3s, 0.15s };

	// ユーティリティ
	void handlePlayerAttack(int32 damage);
	void enemyCounterAttack();
	void finishBattleIfNeeded();
};


