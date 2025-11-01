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
        const bool& crazyMode = targetIsEnemy ? s.enemyCrazyMode : s.playerCrazyMode;
        
        // クレイジーモード中はゲージを増やさない
        if (crazyMode)
        {
            return;
        }
        
        v = Clamp(v + delta, 0, 100);
    }

    // ===== 詠唱システム =====
    
    // 詠唱時間計算
    constexpr double BASE_CAST_TIME = 0.5;
    constexpr double CHAR_CAST_TIME = 0.1;
    
    inline double calculateCastTime(const String& cardName)
    {
        size_t charCount = cardName.length();
        return BASE_CAST_TIME + (charCount * CHAR_CAST_TIME);
    }
    
    // 詠唱開始
    inline bool startCasting(BattleState& s, bool isPlayer, int32 slotIndex, 
                             const String& cardName, double castTime)
    {
        if (isPlayer)
        {
            if (s.playerCasting)
                return false;
            
            s.playerCasting = true;
            s.playerCastTimer.restart();
            s.playerCastDuration = castTime;
            s.playerCastingSlot = slotIndex;
            s.playerCastingCardName = cardName;
        }
        else
        {
            if (s.enemyCasting)
                return false;
            
            s.enemyCasting = true;
            s.enemyCastTimer.restart();
            s.enemyCastDuration = castTime;
            s.enemyCastingSlot = slotIndex;
            s.enemyCastingCardName = cardName;
        }
        
        return true;
    }
    
    // 詠唱完了チェック
    inline bool isCastingComplete(const BattleState& s, bool isPlayer)
    {
        if (isPlayer)
        {
            return s.playerCasting && 
                   (s.playerCastTimer.sF() >= s.playerCastDuration);
        }
        else
        {
            return s.enemyCasting && 
                   (s.enemyCastTimer.sF() >= s.enemyCastDuration);
        }
    }
    
    // 詠唱キャンセル
    inline void cancelCasting(BattleState& s, bool isPlayer)
    {
        if (isPlayer)
        {
            s.playerCasting = false;
            s.playerCastTimer.reset();
            s.playerCastingSlot = -1;
            s.playerCastingCardName.clear();
        }
        else
        {
            s.enemyCasting = false;
            s.enemyCastTimer.reset();
            s.enemyCastingSlot = -1;
            s.enemyCastingCardName.clear();
        }
    }
    
    // 詠唱進行度（0.0～1.0）
    inline double getCastingProgress(const BattleState& s, bool isPlayer)
    {
        if (isPlayer)
        {
            if (!s.playerCasting || s.playerCastDuration <= 0.0)
                return 0.0;
            return Clamp(s.playerCastTimer.sF() / s.playerCastDuration, 0.0, 1.0);
        }
        else
        {
            if (!s.enemyCasting || s.enemyCastDuration <= 0.0)
                return 0.0;
            return Clamp(s.enemyCastTimer.sF() / s.enemyCastDuration, 0.0, 1.0);
        }
    }
    
    // ===== クレイジーモードシステム =====
    
    // クレイジーモード発動チェック
    inline bool shouldEnterCrazyMode(const BattleState& s, bool isPlayer)
    {
        if (isPlayer)
        {
            return !s.playerCrazyMode && (s.playerCrazy >= 100);
        }
        else
        {
            return !s.enemyCrazyMode && (s.enemyCrazy >= 100);
        }
    }
    
    // クレイジーモード開始
    inline void startCrazyMode(BattleState& s, bool isPlayer, double currentTime)
    {
        if (isPlayer)
        {
            s.playerCrazyMode = true;
            s.playerCrazy = 0;
            s.playerCrazyModeStartTime = currentTime;
        }
        else
        {
            s.enemyCrazyMode = true;
            s.enemyCrazy = 0;
            s.enemyCrazyModeStartTime = currentTime;
        }
    }
    
    // クレイジーモード終了チェック
    inline bool shouldExitCrazyMode(const BattleState& s, bool isPlayer, double currentTime)
    {
        if (isPlayer)
        {
            if (!s.playerCrazyMode) return false;
            return (currentTime - s.playerCrazyModeStartTime) >= BattleState::CrazyModeDurationSec;
        }
        else
        {
            if (!s.enemyCrazyMode) return false;
            return (currentTime - s.enemyCrazyModeStartTime) >= BattleState::CrazyModeDurationSec;
        }
    }
    
    // クレイジーモード終了
    inline void endCrazyMode(BattleState& s, bool isPlayer)
    {
        if (isPlayer)
        {
            s.playerCrazyMode = false;
            s.playerCrazyModeStartTime = 0.0;
            s.playerCrazy = 0;  // ゲージをリセット
        }
        else
        {
            s.enemyCrazyMode = false;
            s.enemyCrazyModeStartTime = 0.0;
            s.enemyCrazy = 0;  // ゲージをリセット
        }
    }
}


