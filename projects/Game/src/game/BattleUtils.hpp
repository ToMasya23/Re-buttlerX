# pragma once
# include "BattleState.hpp"

namespace BattleUtils
{
    inline ColorF hpColor(int hp, int maxHP)
    {
        const double r = Clamp(static_cast<double>(hp) / Max(1, maxHP), 0.0, 1.0);
        if (r >= 0.5) return ColorF{ 0.2, 0.8, 0.3 };
        else if (r >= 0.2) return ColorF{ 0.95, 0.85, 0.2 };
        else return ColorF{ 0.9, 0.3, 0.3 };
    }

    inline int32 slotDamage(int slotIndex)
    {
        static constexpr int32 Damage1 = 10;
        static constexpr int32 Damage2 = 20;
        switch (slotIndex)
        {
        case 0: return Damage1;
        case 1: return Damage2;
        case 2: return Damage1 + 5;
        case 3: return Damage2 + 10;
        default: return Damage1;
        }
    }
}


