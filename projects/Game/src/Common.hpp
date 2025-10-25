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

};

using App = SceneManager<State, GameData>;
