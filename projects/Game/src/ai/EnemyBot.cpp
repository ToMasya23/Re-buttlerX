# include "EnemyBot.hpp"
# include "../game/BattleLogic.hpp"

namespace
{
    // 偽装ラベル（クレイジー時に表示用として使用）
    static const Array<String> FakeActionLabels{ U"防御", U"強化", U"回復", U"挑発" };
}

double EnemyBot::castProgress() const
{
    if (!m_casting || (m_castTimeSec <= 0.0)) return 0.0;
    return Clamp(m_castTimer.sF() / m_castTimeSec, 0.0, 1.0);
}

void EnemyBot::regenCostIfAllowed(const BattleState& s, double dt)
{
    const bool blocked = (m_defending || s.waitingForAcknowledge);
    if (!blocked)
    {
        m_costValue = Min(100.0, m_costValue + (CostRegenPerSec * dt));
    }
}

bool EnemyBot::trySpendCost(int32 amount)
{
    const int32 cost = Clamp<int32>(Round(m_costValue), 0, 100);
    if (cost < amount) return false;
    m_costValue = Max(0.0, m_costValue - amount);
    return true;
}

void EnemyBot::startDefend(BattleState& s)
{
    if (m_defending) return;
    if (!trySpendCost(20)) return;
    m_defending = true;
    m_defendTimer.restart();
    // 軽いメッセージを表示（進行一時停止）
    s.battleMessage = U"敵は防御体勢に入った！";
    s.waitingForAcknowledge = true;
    s.nextAction = BattleState::NextAction::BackToSelection;
}

void EnemyBot::startCastAttack(BattleState& s, int32 damage, double castSec, const String& label, const String& displayLabel)
{
    if (m_casting) return;
    if (!trySpendCost(10)) return; // 仮の攻撃コスト
    m_casting = true;
    m_plannedDamage = Max(0, damage);
    m_castTimeSec = Max(0.1, castSec);
    m_plannedLabel = label;
    m_displayedLabel = displayLabel;
    m_castTimer.restart();
}

void EnemyBot::resolveCast(BattleState& s)
{
    m_casting = false;
    const bool playerBlocked = s.defending;
    const int32 dealt = playerBlocked ? 0 : m_plannedDamage;
    s.playerHP = Max(0, s.playerHP - dealt);
    // 敵の攻撃でプレイヤーのクレイジーが増加
    BattleLogic::addCrazy(s, false, +20);
    // 敵はクレイジーを少し発散
    BattleLogic::addCrazy(s, true, -30);
    BattleLogic::startHitEffect(s, BattleState::HitTarget::Player);
    s.battleMessage = (dealt == 0)
        ? U"敵の{}は防がれた！0のダメージ！"_fmt(m_plannedLabel)
        : U"敵は{}を発動！{}のダメージ！"_fmt(m_plannedLabel, dealt);
    s.waitingForAcknowledge = true;
    s.nextAction = BattleState::NextAction::BackToSelection;
}

void EnemyBot::update(BattleState& s, double dt)
{
    // Regen
    regenCostIfAllowed(s, dt);

    // Defend timeout
    if (m_defending && (m_defendTimer.sF() >= EnemyDefendDuration))
    {
        m_defending = false;
    }

    // Casting progress
    if (m_casting)
    {
        if (m_castTimer.sF() >= m_castTimeSec)
        {
            resolveCast(s);
        }
        return; // 詠唱中は新規行動しない
    }

    // If message window is up, don't decide
    if (s.waitingForAcknowledge)
    {
        return;
    }

    // Decision rules
    // Low HP and enough cost -> defend
    if (!m_defending && (s.enemyHP <= 25) && (Clamp<int32>(Round(m_costValue), 0, 100) >= 20))
    {
        startDefend(s);
        return;
    }

    // Attack if enough cost and not defending
    if (!m_defending && (Clamp<int32>(Round(m_costValue), 0, 100) >= 10))
    {
        int32 dmg = 0;
        if (s.enemyCrazy < 60)
        {
            dmg = Random(8, 16);
        }
        else if (s.enemyCrazy < 100)
        {
            dmg = Random(12, 22);
        }
        else
        {
            // クレイジー状態：よりハイリスク/ハイリターン
            dmg = Random(6, 28);
        }
        const double castSec = Random(0.5, 1.4);

        // ラベル（実際と表示）：クレイジー時はあべこべ表示
        const String realLabel = U"攻撃";
        String displayLabel = realLabel;
        if (s.enemyCrazy >= 100)
        {
            displayLabel = FakeActionLabels.choice();
        }

        startCastAttack(s, dmg, castSec, realLabel, displayLabel);
        return;
    }
}
