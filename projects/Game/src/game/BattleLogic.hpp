# pragma once
# include "BattleState.hpp"

namespace BattleLogic
{
    inline void startHitEffect(BattleState& s, BattleState::HitTarget target)
    {
        s.hitTarget = target;
        s.hitTimer.restart();
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
        return s.defending;
    }

    inline bool canAttack(const BattleState& s)
    {
        return !s.defending;
    }

    inline bool canDefend(const BattleState& s)
    {
        if (s.defending) return false;
        return (s.cost() >= 20);
    }

    inline void addCrazy(BattleState& s, bool targetIsEnemy, int32 delta)
    {
        int32& v = targetIsEnemy ? s.enemyCrazy : s.playerCrazy;
        v = Clamp(v + delta, 0, 100);
    }
}


