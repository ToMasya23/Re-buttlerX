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

	// ---- カード関連（cards.json） ----
	struct CardSpec
	{
		String id;
		String name;
		int32 cost = 0;       // 使用時コスト
		double delaySec = 0;   // 詠唱時間（秒）
		double weight = 1.0;   // 抽選重み
	};

	Array<CardSpec> m_allCards;      // 全カード
	Array<CardSpec> m_currentCards;  // 現在表示中（最大4）
	Array<CardSpec> m_lastDisplayedCards; // 直前に表示していた4枚（インターバル中の表示用）

	// 直前に使用したスロットとカードID
	int32 m_lastUsedSlot = -1;
	String m_lastUsedCardId;

	// 使用後の一時的な出現禁止（3秒）
	struct CardCooldown { String id; Stopwatch timer{ StartImmediately::Yes }; };
	Array<CardCooldown> m_cardCooldowns;
	static constexpr double PerCardCooldownSec = 3.0;

	// 詠唱＆リフィル制御
	bool m_casting = false;
	double m_currentCastDurationSec = 0.0;
	Stopwatch m_castTimer{ StartImmediately::No };
	int32 m_pendingDamage = 0; // 詠唱完了後に与えるダメージ

	bool m_inInterval = false;
	Stopwatch m_intervalTimer{ StartImmediately::No };
	static constexpr double IntervalSec = 3.0;

	// カード操作ヘルパー
	void loadCardsFromJSON();
	void refillRandomCards();
	bool hasCards() const { return (not m_allCards.empty()); }
	int32 slotDamage(int slotIndex) const; // スロットに紐づく固定ダメージ
	void cleanupCooldowns();
	Optional<CardSpec> pickRandomCardExcluding(const Array<String>& excludeIds) const;
	void replaceUsedCard();
};


