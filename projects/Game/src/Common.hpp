# pragma once
# include <Siv3D.hpp>

// シーンのステート
enum class State
{
	Title,
    Menu,
    Battle,
    Game,
    Lobby,
    Matching,
    Result,
    PauseOverlay,
    Settings,
    HowToPlay,
    EffectViewer,
};

// 共有するデータ
struct GameData
{
	// 直前のゲームのスコア
	int32 lastScore = 0;

    // ゲームモード
    enum class GameMode
    {
        Unknown,
        PvP,
        PvE,
    };

    // 直前にプレイしたモード
    GameMode lastMode = GameMode::Unknown;

    // 一時停止（ポーズ）から戻るための直前シーン
    State previousState = State::Title;

	// ポーズ時に下層シーンを描画するための背景
	DynamicTexture pauseBackground;
};

using App = SceneManager<State, GameData>;
