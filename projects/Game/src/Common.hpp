# pragma once
# include <Siv3D.hpp>
# include <memory>

// Forward declaration
class MultiplayerManager;

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

    // オンライン対戦用
    std::shared_ptr<MultiplayerManager> multiplayer;
    bool isHost = false;

};

using App = SceneManager<State, GameData>;
