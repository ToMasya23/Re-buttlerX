# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
# include "../ui/BattleLayout.hpp"
# include "../game/BattleState.hpp"
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
    // ---- バトル状態（PvE/PvP共通） ----
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
};


