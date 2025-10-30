# pragma once
# include "../Common.hpp"
# include "../ui/PauseTheme.hpp"
# include "../ui/PauseMenu.hpp"
# include "../ui/BattleLayout.hpp"
# include "../game/BattleState.hpp"
# include "../game/FaceTextures.hpp"
# include "../game/CardDeck.hpp"

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
};


