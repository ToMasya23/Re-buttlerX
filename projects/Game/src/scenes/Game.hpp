# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
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

	// 攻撃メッセージ＆入力待機
	bool m_waitingForAcknowledge = false;
	String m_battleMessage;
	enum class NextAction { None, EnemyCounter, BackToSelection, FinishBattle };
	NextAction m_nextAction = NextAction::None;

	// 被弾エフェクト
	enum class HitTarget { None, Player, Enemy };
	HitTarget m_hitTarget = HitTarget::None;
	Stopwatch m_hitTimer{ StartImmediately::No };
	static constexpr double HitDuration = 0.25; // seconds

	// ---- ポーズ用 ----
	bool m_paused = false;
	bool m_pauseAwaitingCapture = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

    PauseMenu m_pauseMenu;

	// ボタンのホバー演出
	Transition m_attackTr{ 0.3s, 0.15s };
	Transition m_escapeTr{ 0.3s, 0.15s };
	Transition m_attack1Tr{ 0.3s, 0.15s };
	Transition m_attack2Tr{ 0.3s, 0.15s };

	// ユーティリティ
	void handlePlayerAttack(int32 damage);
	void doEnemyCounterStep();
	void finishBattleIfNeeded();
	void startHitEffect(HitTarget target);
	bool advanceInputDown() const;
};


