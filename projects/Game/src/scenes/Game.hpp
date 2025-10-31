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

	// コスト（内部は double で管理）
	double m_costValue = 100.0; // 0..100
	static constexpr double CostRegenPerSec = 1.0; // 1/sec
	int32 cost() const { return Clamp<int32>(Round(m_costValue), 0, 100); }

	// 防御
	bool m_defending = false;
	Stopwatch m_defendTimer{ StartImmediately::No };
	static constexpr double DefendDurationSec = 5.0;

	// クレイジーゲージ（0..100）
	int32 m_playerCrazy = 0;
	int32 m_enemyCrazy = 0;

    // 顔テクスチャ
    s3d::Texture m_texSmile;
    s3d::Texture m_texMagao;
    s3d::Texture m_texCloudy;
    s3d::Texture m_texCrying;

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

	// ---- 敵（PvE）用 簡易 AI 状態 ----
	// コスト（内部は double で管理）
	double m_enemyCostValue = 100.0; // 0..100
	static constexpr double EnemyCostRegenPerSec = 1.0; // 1/sec（プレイヤーと同等）
	int32 enemyCost() const { return Clamp<int32>(Round(m_enemyCostValue), 0, 100); }

	// 防御状態
	bool m_enemyDefending = false;
	Stopwatch m_enemyDefendTimer{ StartImmediately::No };

	// 詠唱（攻撃準備）
	bool m_enemyCasting = false;
	double m_enemyCastTimeSec = 0.0;
	Stopwatch m_enemyCastTimer{ StartImmediately::No };
	int32 m_enemyPlannedDamage = 0;
	String m_enemyPlannedLabel;          // 実際の効果名称（攻撃/防御 等）
	String m_enemyDisplayedLabel;        // 表示用（クレイジー時はあべこべ）

	// クレイジー時の挙動（100% 以上で “あべこべ表示” を発動）
	bool enemyInCrazy() const { return (m_enemyCrazy >= 100); }

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

	// ---- 敵 AI 用ヘルパー ----
	void enemyRegenCost(double dt);
	bool enemyIsRegenBlocked() const;
	bool enemyTrySpendCost(int32 amount);
	void enemyUpdateAI();
	void enemyStartDefend();
	void enemyStartCastAttack(int32 damage, double castSec, const String& label, const String& displayLabel);
	void enemyResolveCast();
};


