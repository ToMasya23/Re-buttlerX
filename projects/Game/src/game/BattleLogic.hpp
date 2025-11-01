# pragma once
# include "BattleState.hpp"

namespace BattleLogic
{
    inline void startHitEffect(BattleState& s, BattleState::HitTarget target)
    {
        s.hitTarget = target;
        s.hitTimer.restart();
    }

    inline bool advanceInputDown()
    {
        return (MouseL.down() || KeyEnter.down() || KeySpace.down() || KeyZ.down() || KeyX.down());
    }

    inline void regenCost(BattleState& s, double dt)
    {
        s.costValue = Min(100.0, s.costValue + (BattleState::CostRegenPerSec * dt));
    }

    inline bool trySpendCost(BattleState& s, int32 amount)
    {
        if (s.cost() < amount) return false;
        s.costValue = Max(0.0, s.costValue - amount);
        return true;
    }

    inline bool isRegenBlocked(const BattleState& s)
    {
        return (s.defending || s.waitingForAcknowledge);
    }

    inline bool canAttack(const BattleState& s)
    {
        if (s.defending || s.waitingForAcknowledge) return false;
        return true;
    }

    inline bool canDefend(const BattleState& s)
    {
        if (s.defending || s.waitingForAcknowledge) return false;
        return (s.cost() >= 20);
    }

    inline void addCrazy(BattleState& s, bool targetIsEnemy, int32 delta)
    {
        int32& v = targetIsEnemy ? s.enemyCrazy : s.playerCrazy;
        v = Clamp(v + delta, 0, 100);
    }

    inline void handlePlayerAttack(BattleState& s, int32 damage)
    {
        s.enemyHP = Max(0, s.enemyHP - damage);
        startHitEffect(s, BattleState::HitTarget::Enemy);
        s.battleMessage = U"プレイヤーは敵に攻撃した！{}のダメージを与えた！"_fmt(damage);
        s.waitingForAcknowledge = true;
        addCrazy(s, true, +20);
        addCrazy(s, false, -10);
        s.nextAction = (s.enemyHP <= 0) ? BattleState::NextAction::FinishBattle
                                        : BattleState::NextAction::EnemyCounter;
    }

    inline void enemyCounter(BattleState& s)
    {
        const int32 enemyDamage = Random(8, 16);
        int32 finalDamage = s.defending ? 0 : enemyDamage;
        s.playerHP = Max(0, s.playerHP - finalDamage);
        addCrazy(s, false, +20);
        startHitEffect(s, BattleState::HitTarget::Player);
        s.battleMessage = (finalDamage == 0)
            ? U"敵は攻撃したが、防御した！0のダメージ！"
            : U"敵はプレイヤーに攻撃した！{}のダメージを与えた！"_fmt(finalDamage);
        s.waitingForAcknowledge = true;
        s.nextAction = (s.playerHP <= 0) ? BattleState::NextAction::FinishBattle
                                         : BattleState::NextAction::BackToSelection;
    }
}


