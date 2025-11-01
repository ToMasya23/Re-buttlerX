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

    // メッセージ・進行
    bool waitingForAcknowledge = false;
    String battleMessage;
    enum class NextAction { None, EnemyCounter, BackToSelection, FinishBattle };
    NextAction nextAction = NextAction::None;

    // 被弾エフェクト
    enum class HitTarget { None, Player, Enemy };
    HitTarget hitTarget = HitTarget::None;
    Stopwatch hitTimer{ StartImmediately::No };
    static constexpr double HitDuration = 0.25;
};


