# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
# include "../ui/BattleLayout.hpp"
# include "../game/BattleState.hpp"
# include "../game/FaceTextures.hpp"
# include "../game/CardDeck.hpp"
# include "../tools/AudioManager.hpp"
# include "../ai/EnemyBot.hpp"

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
	EnemyBot m_enemy;

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

    // ---- 敵 AI は EnemyBot に委譲 ----
};


