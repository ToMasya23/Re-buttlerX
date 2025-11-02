# pragma once
# include "../Common.hpp"
# include "../game/BattleState.hpp"

// Simple PvE enemy AI extracted from Game scene
class EnemyBot
{
public:
    // Update AI state and possibly modify BattleState (messages, HP, nextAction)
    void update(BattleState& s, double dt);

    // Read-only status for UI overlays
    bool isDefending() const { return m_defending; }
    bool isCasting()   const { return m_casting; }
    double castProgress() const; // 0..1
    const String& displayedLabel() const { return m_displayedLabel; }

private:
    // Internal helpers
    void regenCostIfAllowed(const BattleState& s, double dt);
    bool trySpendCost(int32 amount);
    void startDefend(BattleState& s);
    void startCastAttack(BattleState& s, int32 damage, double castSec, const String& label, const String& displayLabel);
    void resolveCast(BattleState& s);

    // Enemy-side resources
    double m_costValue = 100.0; // 0..100
    static constexpr double CostRegenPerSec = 1.0; // same as player cost regen

    // Defend state
    bool m_defending = false;
    Stopwatch m_defendTimer{ StartImmediately::No };
    static constexpr double EnemyDefendDuration = 3.0;

    // Casting state
    bool m_casting = false;
    double m_castTimeSec = 0.0;
    Stopwatch m_castTimer{ StartImmediately::No };
    int32 m_plannedDamage = 0;
    String m_plannedLabel;    // actual effect name ("攻撃" etc.)
    String m_displayedLabel;  // UI label (can be faked when crazy)
};
