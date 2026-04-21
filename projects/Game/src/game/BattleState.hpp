# pragma once
# include "../Common.hpp"

// 戦闘の進行・数値状態
struct BattleState
{
    static constexpr int32 MaxHP = 100;

    int32 playerHP = MaxHP;
    int32 enemyHP = MaxHP;

    // コスト（0..100、内部 double）
    double costValue = 100.0;
    static constexpr double CostRegenPerSec = 1.0;
    int32 cost() const { return Clamp<int32>(Round(costValue), 0, 100); }

    // 防御
    bool defending = false;
    Stopwatch defendTimer{ StartImmediately::No };
    static constexpr double DefendDurationSec = 5.0;

    // クレイジーゲージ
    int32 playerCrazy = 0;
    int32 enemyCrazy = 0;

    // ===== クレイジーモード =====
    bool playerCrazyMode = false;
    bool enemyCrazyMode = false;
    double playerCrazyModeStartTime = 0.0;
    double enemyCrazyModeStartTime = 0.0;
    static constexpr double CrazyModeDurationSec = 15.0;

    // メッセージ
    String battleMessage;

    // 被弾エフェクト
    enum class HitTarget { None, Player, Enemy };
    HitTarget hitTarget = HitTarget::None;
    Stopwatch hitTimer{ StartImmediately::No };
    bool hitIsWeakness = false;
    static constexpr double HitDuration = 0.5;

    // ===== 詠唱システム =====
    bool playerCasting = false;
    bool enemyCasting = false;
    Stopwatch playerCastTimer{ StartImmediately::No };
    Stopwatch enemyCastTimer{ StartImmediately::No };
    double playerCastDuration = 0.0;
    double enemyCastDuration = 0.0;
    int32 playerCastingSlot = -1;
    int32 enemyCastingSlot = -1;
    String playerCastingCardName;
    String enemyCastingCardName;

    // ===== 属性システム =====
    int32 playerAttributeId = 0;  // プレイヤーの現在属性（0=デフォルト）
    int32 enemyAttributeId = 0;   // 敵の現在属性（0=デフォルト）
};


