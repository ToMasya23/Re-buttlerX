# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
# include "../ui/BattleLayout.hpp"
# include "../game/PlayerState.hpp"
# include "../network/MultiplayerManager.hpp"
# include "../network/BattleMessages.hpp"

// ゲームシーン（PvE / PvP バトル）
class Game : public App::Scene
{
public:

	Game(const InitData& init);

	void update() override;

	void draw() const override;

private:

	// ---- バトル用（PlayerStateで統一） ----
	static constexpr int32 MaxHP = BattleConstants::MaxHP;
	PlayerState m_player1;  // ローカルプレイヤー（またはホスト）
	PlayerState m_player2;  // 敵AI（またはリモートプレイヤー）

	// ===== オンライン対戦用 =====
	std::shared_ptr<MultiplayerManager> m_multiplayer;
	bool m_isOnlineMode = false;  // オンラインモードか
	bool m_isHost = false;         // ホストか
	bool m_isMyTurn = false;       // 自分のターンか
	uint32 m_turnNumber = 0;       // ターン番号

	// 後方互換性のためのヘルパー（既存コードを最小限の変更で動作させる）
	int32& m_playerHP = m_player1.hp;
	int32& m_enemyHP = m_player2.hp;
	double& m_costValue = m_player1.costValue;
	bool& m_defending = m_player1.defending;
	Stopwatch& m_defendTimer = m_player1.defendTimer;
	int32& m_playerCrazy = m_player1.crazyGauge;
	int32& m_enemyCrazy = m_player2.crazyGauge;
	int32 cost() const { return m_player1.getCost(); }

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

	// コスト・防御ヘルパー
	void regenCost(double dt);
	int32 calcAttackCost(const String& label) const;
	bool trySpendCost(int32 amount);
	bool isRegenBlocked() const;
	bool canAttack(const String& label) const;
	bool canDefend() const;

	// クレイジー関連
	void addCrazy(bool targetIsEnemy, int32 delta);
	static ColorF hpColor(int hp, int maxHP);
    const s3d::Texture& selectFaceTexture(int crazyPercent) const;

	// ===== オンライン対戦用ヘルパー =====
	void handleNetworkMessages();
	void sendPlayerAction(ActionType action);
	void handleOpponentAction(const PlayerActionMessage& msg);
	void sendGameStateSync();  // 現在のゲーム状態を相手に送信
	void syncGameState(const GameStateSyncMessage& msg);
	void doLocalEnemyCounter();  // ローカルAI（PvE用）
	int32 calculateDamage(ActionType action) const;
};


