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

// 前方宣言
class MultiplayerManager;

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

    // ===== オンライン対戦用 =====
    std::shared_ptr<MultiplayerManager> multiplayer;  // マルチプレイヤー管理
    bool isHost = false;                               // ホストかどうか

};

using App = SceneManager<State, GameData>;
