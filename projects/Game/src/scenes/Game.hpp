# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
# include "../ui/BattleLayout.hpp"
# include "../game/BattleState.hpp"
# include "../game/PlayerState.hpp"
# include "../game/FaceTextures.hpp"
# include "../game/CardDeck.hpp"
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
    // ---- バトル状態（PvE用） ----
    BattleState m_state;
    FaceTextures m_faces;
    CardDeck m_deck;

	// ---- キャラクタ表示用テクスチャ ----
	s3d::Texture m_texPlayer;
	s3d::Texture m_texEnemy;

	// ===== オンライン対戦用 =====
	std::shared_ptr<MultiplayerManager> m_multiplayer;
	bool m_isOnlineMode = false;
	bool m_isHost = false;
	bool m_isMyTurn = false;
	uint32 m_turnNumber = 0;
	
	// PvP用のプレイヤー状態（サーバー同期用）
	PlayerState m_player1;  // ローカルプレイヤー（またはホスト）
	PlayerState m_player2;  // 敵AI（またはリモートプレイヤー）
	
	// PvPモード用の状態変数
	bool m_waitingForAcknowledge = false;
	String m_battleMessage;
	enum class NextAction { None, EnemyCounter, BackToSelection, FinishBattle };
	NextAction m_nextAction = NextAction::None;
	
	// 被弾エフェクト
	enum class HitTarget { None, Player, Enemy };
	HitTarget m_hitTarget = HitTarget::None;
	Stopwatch m_hitTimer{ StartImmediately::No };
	static constexpr double HitDuration = 0.25;
	
	// ボタンホバー用トランジション
	Transition m_attack1Tr{ 0.3s, 0.15s };
	Transition m_attack2Tr{ 0.3s, 0.15s };

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
    
    // オンライン対戦用ヘルパー
    void handleNetworkMessages();
    void sendGameStateSync();
    void sendPlayerAction(ActionType action);
    
    // PvPモード用のヘルパー関数
    bool advanceInputDown() const;
    bool isRegenBlocked() const;
    void regenCost(double dt);
    bool canAttack(const String& actionName) const;
    bool trySpendCost(int32 amount);
    void handlePlayerAttack(int32 damage);
    void doLocalEnemyCounter();
    void doEnemyCounterStep();
};


