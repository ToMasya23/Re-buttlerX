# pragma once
# include "BattleState.hpp"

// Forward declaration
class CardDeck;

namespace BattleUtils
{
    inline ColorF hpColor(int hp, int maxHP)
    {
        const double r = Clamp(static_cast<double>(hp) / Max(1, maxHP), 0.0, 1.0);
        if (r >= 0.5) return ColorF{ 0.2, 0.8, 0.3 };
        else if (r >= 0.2) return ColorF{ 0.95, 0.85, 0.2 };
        else return ColorF{ 0.9, 0.3, 0.3 };
    }

    // 文字数ベースのダメージ計算
    inline int32 calculateDamage(const String& cardName)
    {
        constexpr int32 BASE_DAMAGE = 3;
        constexpr int32 DAMAGE_PER_CHAR = 1.5;
        size_t charCount = cardName.length();
        return BASE_DAMAGE + (static_cast<int32>(charCount) * DAMAGE_PER_CHAR);
    }

    // スロットのダメージ取得（CardDeckを参照）
    // 実装はCardDeck.hppの後に定義
    inline int32 slotDamage(int slotIndex, const CardDeck& deck);
}

// CardDeck.hppをインクルード後に実装
#include "CardDeck.hpp"

namespace BattleUtils
{
    inline int32 slotDamage(int slotIndex, const CardDeck& deck)
    {
        const CardSpec& actualCard = deck.getActualCard(slotIndex);
        return actualCard.damageHP;
    }
}


