# pragma once
# include "CardRepository.hpp"

// カードの場・クールダウン管理
class CardDeck
{
public:
    struct CardCooldown { String id; Stopwatch timer{ StartImmediately::Yes }; };

    static constexpr double PerCardCooldownSec = 3.0;

    void loadAll()
    {
        m_allCards = Cards::loadFromJSON();
    }

    bool hasCards() const { return (not m_allCards.empty()); }

    const Array<CardSpec>& current() const { return m_currentCards; }
    const Array<CardSpec>& lastDisplayed() const { return m_lastDisplayedCards; }
    bool hasPendingReplacement() const { return (m_lastUsedSlot >= 0); }

    void refillRandom(int maxSlots = 4)
    {
        m_lastDisplayedCards = m_currentCards;
        m_currentCards.clear();
        if (m_allCards.isEmpty()) return;

        Array<int32> indices(m_allCards.size());
        for (size_t i = 0; i < indices.size(); ++i) indices[i] = static_cast<int32>(i);

        const int k = Min<int>(maxSlots, static_cast<int>(indices.size()));
        for (int pick = 0; pick < k; ++pick)
        {
            double sum = 0.0;
            for (const auto idx : indices)
            {
                sum += Max(0.0, m_allCards[idx].weight);
            }
            int chosenLocal = 0;
            if (sum <= 0.0)
            {
                chosenLocal = Random(0, static_cast<int>(indices.size()) - 1);
            }
            else
            {
                double r = Random(0.0, sum);
                double acc = 0.0;
                for (int i = 0; i < static_cast<int>(indices.size()); ++i)
                {
                    acc += Max(0.0, m_allCards[indices[i]].weight);
                    if (r <= acc)
                    {
                        chosenLocal = i;
                        break;
                    }
                }
            }
            const int32 chosenIndex = indices[chosenLocal];
            m_currentCards << m_allCards[chosenIndex];
            indices.remove_at(chosenLocal);
        }
    }

    void cleanupCooldowns()
    {
        m_cardCooldowns.remove_if([&](const CardCooldown& cd){ return cd.timer.sF() >= PerCardCooldownSec; });
    }

    void onUse(int32 slotIndex)
    {
        if (slotIndex < 0 || slotIndex >= static_cast<int32>(m_currentCards.size())) return;
        m_lastUsedSlot = slotIndex;
        m_lastUsedCardId = m_currentCards[slotIndex].id;
        m_cardCooldowns << CardCooldown{ m_lastUsedCardId };
    }

    void replaceUsedCard()
    {
        if (m_lastUsedSlot < 0 || m_lastUsedSlot >= static_cast<int32>(m_currentCards.size()))
        {
            return;
        }
        Array<String> exclude;
        exclude << m_lastUsedCardId;
        for (int i = 0; i < static_cast<int>(m_currentCards.size()); ++i)
        {
            if (i == m_lastUsedSlot) continue;
            exclude << m_currentCards[i].id;
        }

        Optional<CardSpec> picked = pickRandomCardExcluding(exclude);
        if (!picked)
        {
            Array<int32> candidates;
            for (int32 i = 0; i < static_cast<int32>(m_allCards.size()); ++i)
            {
                if (exclude.includes(m_allCards[i].id)) continue;
                candidates << i;
            }
            if (!candidates.isEmpty())
            {
                const int r = Random(0, static_cast<int>(candidates.size()) - 1);
                picked = m_allCards[candidates[r]];
            }
        }
        if (picked)
        {
            m_currentCards[m_lastUsedSlot] = *picked;
            m_lastDisplayedCards = m_currentCards;
        }
        m_lastUsedSlot = -1;
        m_lastUsedCardId.clear();
    }

    Optional<CardSpec> pickRandomCardExcluding(const Array<String>& excludeIds) const
    {
        Array<int32> candidates;
        candidates.reserve(m_allCards.size());
        for (int32 i = 0; i < static_cast<int32>(m_allCards.size()); ++i)
        {
            const auto& c = m_allCards[i];
            if (excludeIds.includes(c.id)) continue;
            bool onCD = false;
            for (const auto& cd : m_cardCooldowns)
            {
                if (cd.id == c.id && cd.timer.sF() < PerCardCooldownSec) { onCD = true; break; }
            }
            if (onCD) continue;
            candidates << i;
        }
        if (candidates.isEmpty())
        {
            return none;
        }
        double sum = 0.0;
        for (const auto idx : candidates) sum += Max(0.0, m_allCards[idx].weight);
        if (sum <= 0.0)
        {
            const int r = Random(0, static_cast<int>(candidates.size()) - 1);
            return m_allCards[candidates[r]];
        }
        double r = Random(0.0, sum);
        double acc = 0.0;
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
        {
            acc += Max(0.0, m_allCards[candidates[i]].weight);
            if (r <= acc)
            {
                return m_allCards[candidates[i]];
            }
        }
        return m_allCards[candidates.back()];
    }

private:
    Array<CardSpec> m_allCards;
    Array<CardSpec> m_currentCards;
    Array<CardSpec> m_lastDisplayedCards;
    Array<CardCooldown> m_cardCooldowns;
    int32 m_lastUsedSlot = -1;
    String m_lastUsedCardId;
};


