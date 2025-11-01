# pragma once
# include "CardRepository.hpp"

// カードの場・クールダウン管理
class CardDeck
{
public:
    struct CardCooldown { String id; Stopwatch timer{ StartImmediately::Yes }; };
    struct SlotRefill
    {
        int32 slot;
        Stopwatch timer{ StartImmediately::Yes };
        CardSpec pendingCard;
    };

    static constexpr double PerCardCooldownSec = 3.0;
    static constexpr double RefillCooldownSec = 3.0;

    void loadAll()
    {
        m_allCards = Cards::loadFromJSON();
    }

    bool hasCards() const { return (not m_allCards.empty()); }

    const Array<CardSpec>& current() const { return m_currentCards; }
    const Array<CardSpec>& lastDisplayed() const { return m_lastDisplayedCards; }
    bool hasPendingReplacement() const { return (m_lastUsedSlot >= 0); }
    
    bool isSlotRefilling(int32 slot) const
    {
        for (const auto& refill : m_pendingRefills)
        {
            if (refill.slot == slot)
            {
                return true;
            }
        }
        return false;
    }
    
    double getSlotRefillProgress(int32 slot) const
    {
        for (const auto& refill : m_pendingRefills)
        {
            if (refill.slot == slot)
            {
                return Min(1.0, refill.timer.sF() / RefillCooldownSec);
            }
        }
        return 1.0;
    }

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
    
    void updateRefills()
    {
        for (int i = static_cast<int>(m_pendingRefills.size()) - 1; i >= 0; --i)
        {
            auto& refill = m_pendingRefills[i];
            
            if (refill.timer.sF() >= RefillCooldownSec)
            {
                if (refill.slot >= 0 && refill.slot < static_cast<int>(m_currentCards.size()))
                {
                    m_currentCards[refill.slot] = refill.pendingCard;
                    m_lastDisplayedCards = m_currentCards;
                }
                
                m_pendingRefills.remove_at(i);
            }
        }
    }
    
    void requestRefill(int32 slot, const CardSpec& newCard)
    {
        for (auto& refill : m_pendingRefills)
        {
            if (refill.slot == slot)
            {
                refill.timer.restart();
                refill.pendingCard = newCard;
                return;
            }
        }
        
        SlotRefill refill;
        refill.slot = slot;
        refill.timer = Stopwatch{ StartImmediately::Yes };
        refill.pendingCard = newCard;
        m_pendingRefills << refill;
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
            requestRefill(m_lastUsedSlot, *picked);
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

    void enterCrazyMode()
    {
        m_crazyMode = true;
        
        if (m_allCards.isEmpty()) return;

        Array<int32> indices(m_allCards.size());
        for (size_t i = 0; i < indices.size(); ++i) indices[i] = static_cast<int32>(i);

        Array<CardSpec> newCards;
        const int k = Min<int>(4, static_cast<int>(indices.size()));
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
            newCards << m_allCards[chosenIndex];
            indices.remove_at(chosenLocal);
        }
        
        for (int i = 0; i < static_cast<int>(newCards.size()); ++i)
        {
            requestRefill(i, newCards[i]);
        }
        
        m_visualCards.clear();
        for (size_t i = 0; i < newCards.size(); ++i)
        {
            Array<int32> candidates;
            for (int32 j = 0; j < static_cast<int32>(m_allCards.size()); ++j)
            {
                if (m_allCards[j].id != newCards[i].id)
                {
                    candidates << j;
                }
            }
            
            if (!candidates.isEmpty())
            {
                const int r = Random(0, static_cast<int>(candidates.size()) - 1);
                m_visualCards << m_allCards[candidates[r]];
            }
            else
            {
                m_visualCards << newCards[i];
            }
        }
    }
    
    void exitCrazyMode()
    {
        m_crazyMode = false;
        m_visualCards.clear();
    }
    
    bool isCrazyMode() const
    {
        return m_crazyMode;
    }
    
    const CardSpec& getVisualCard(size_t index) const
    {
        if (m_crazyMode && index < m_visualCards.size())
        {
            return m_visualCards[index];
        }
        return m_currentCards[index];
    }
    
    const CardSpec& getActualCard(size_t index) const
    {
        return m_currentCards[index];
    }

private:
    Array<CardSpec> m_allCards;
    Array<CardSpec> m_currentCards;
    Array<CardSpec> m_lastDisplayedCards;
    Array<CardCooldown> m_cardCooldowns;
    int32 m_lastUsedSlot = -1;
    String m_lastUsedCardId;
    
    bool m_crazyMode = false;
    Array<CardSpec> m_visualCards;
    
    Array<SlotRefill> m_pendingRefills;
};


