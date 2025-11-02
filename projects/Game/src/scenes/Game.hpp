# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
# include "../ui/BattleLayout.hpp"
# include "../game/BattleState.hpp"
# include "../game/FaceTextures.hpp"
# include "../game/CardDeck.hpp"
# include "../tools/AudioManager.hpp"

// ゲームシーン（PvE バトル）
class Game : public App::Scene
{
public:

	Game(const InitData& init);

	void update() override;

	void draw() const override;

private:
    // ---- バトル状態 ----
    BattleState m_state;
    FaceTextures m_faces;
    CardDeck m_deck;

	// ---- キャラクタ表示用テクスチャ ----
	s3d::Texture m_texPlayer;
	s3d::Texture m_texEnemy;

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
	bool enemyInCrazy() const { return (m_state.enemyCrazy >= 100); }

	// ---- ポーズ用 ----
	bool m_paused = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

    PauseMenu m_pauseMenu;

	// ボタンのホバー演出（必要なもののみ）
	Transition m_escapeTr{ 0.3s, 0.15s };

    // ユーティリティ
    void finishBattleIfNeeded();

	// ---- 敵 AI 用ヘルパー ----
	void enemyRegenCost(double dt);
	bool enemyIsRegenBlocked() const;
	bool enemyTrySpendCost(int32 amount);
	void enemyUpdateAI();
	void enemyStartDefend();
	void enemyStartCastAttack(int32 damage, double castSec, const String& label, const String& displayLabel);
	void enemyResolveCast();
};


