# pragma once
# include <Siv3D.hpp>
# include <memory>

// Forward declaration
class MultiplayerManager;

// シーンのステート
enum class State
{
	Title,
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
    // ゲームモード
    enum class GameMode
    {
        Unknown,
        PvP,
        PvE,
    };

    // 直前にプレイしたモード
    GameMode lastMode = GameMode::Unknown;

    // オンライン対戦用
    std::shared_ptr<MultiplayerManager> multiplayer;
    bool isHost = false;

	State pauseReturnState;
};

using App = SceneManager<State, GameData>;
