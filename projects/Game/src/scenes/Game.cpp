// Header & helpers
# include "Game.hpp"
# include "../game/BattleLogic.hpp"
# include "../game/BattleUtils.hpp"
# include "../tools/NineSlice.hpp"

namespace
{
    static constexpr int32 Damage1 = 10;
    static constexpr int32 Damage2 = 20;

    NineSliceSkin& ScreenFrame()
    {
        static NineSliceSkin skin{
            U"assets/ui/frames/battle_frame.png",
            20, 20, 20, 20,
            false
        };
        return skin;
    }

    NineSliceSkin& BaseFrame()
    {
        static NineSliceSkin skin{
            U"assets/ui/frames/battle_base.png",
            20, 20, 20, 20,
            false
        };
        return skin;
    }

    inline void drawFit(const s3d::Texture& tex, const s3d::RectF& dst, const s3d::ColorF& tint = s3d::Palette::White)
    {
        const s3d::ScopedRenderStates2D _nn{ s3d::SamplerState::ClampNearest };
        const double sx = dst.w / tex.width();
        const double sy = dst.h / tex.height();
        const double s = s3d::Min(sx, sy);
        const s3d::Vec2 size = s3d::Vec2{ tex.width(), tex.height() } * s;
        const s3d::Vec2 pos = dst.center() - size * 0.5;
        tex.scaled(s).draw(pos, tint);
    }

    // 敵がクレイジー状態の時の偽装ラベル候補
    static const Array<String> FakeActionLabels{ U"防御", U"強化", U"回復", U"挑発" };
}

Game::Game(const InitData& init)
    : IScene{ init }
{
    m_faces.load();
    m_deck.loadAll();
    if (m_deck.hasCards())
    {
        m_deck.refillRandom(4);
    }

    // キャラクタテクスチャの読み込み（ドットのにじみを避けるため Unmipped）
    m_texPlayer = s3d::Texture{ U"assets/ui/characters/player.png", s3d::TextureDesc::Unmipped };
    m_texEnemy  = s3d::Texture{ U"assets/ui/characters/enemy.png",  s3d::TextureDesc::Unmipped };

    AudioManager::instance().startBGM(U"assets/BGM/menu.mp3", 0.7);
}

void Game::update()
{
    // Esc でポーズをトグル
    if (KeyEscape.down())
    {
        m_paused = (not m_paused);
        return;
    }

    // 使用後のクールダウン掃除
    m_deck.cleanupCooldowns();

    if (m_paused)
    {
        const PauseMenu::Action action = m_pauseMenu.update();
        switch (action)
        {
        case PauseMenu::Action::Resume:
            m_paused = false;
            break;
        case PauseMenu::Action::Settings:
            changeScene(State::Settings);
            break;
        case PauseMenu::Action::HowToPlay:
            changeScene(State::HowToPlay);
            break;
        case PauseMenu::Action::EffectViewer:
            changeScene(State::EffectViewer);
            break;
        case PauseMenu::Action::Title:
            changeScene(State::Title);
            break;
        case PauseMenu::Action::Exit:
            System::Exit();
            break;
        default:
            break;
        }
        return;
    }

    // コスト回復（停止条件を考慮）
    if (!BattleLogic::isRegenBlocked(m_state))
    {
        BattleLogic::regenCost(m_state, Scene::DeltaTime());
    }
    // 敵コスト回復（停止条件を考慮）
    if (!enemyIsRegenBlocked())
    {
        enemyRegenCost(Scene::DeltaTime());
    }

    // 防御の継続時間チェック
    if (m_state.defending && (m_state.defendTimer.sF() >= BattleState::DefendDurationSec))
    {
        m_state.defending = false;
    }

    // ---- メッセージ待機中は進行を止める（AI 進行も止める） ----
    if (m_state.waitingForAcknowledge)
    {
        if (BattleLogic::advanceInputDown())
        {
            m_state.waitingForAcknowledge = false;
            if (m_state.nextAction == BattleState::NextAction::EnemyCounter)
            {
                // 連続カウンターは廃止（AI が自律的に行動）
            }
            else if (m_state.nextAction == BattleState::NextAction::FinishBattle)
            {
                finishBattleIfNeeded();
            }
            else if (m_state.nextAction == BattleState::NextAction::BackToSelection)
            {
                // そのまま選択に戻る（使用カードを1枚だけ置き換え）
                m_deck.replaceUsedCard();
            }
            m_state.nextAction = BattleState::NextAction::None;
        }
        return;
    }

    // 念のため：待機中でなく、置換待ちが残っていればここで実行
    if (m_deck.hasPendingReplacement())
    {
        m_deck.replaceUsedCard();
    }

    // ---- カード補充（起動直後など空のとき） ----
    if (m_deck.current().isEmpty() && m_deck.hasCards())
    {
        m_deck.refillRandom(4);
    }

    // ---- PvE バトル更新 ----
    const Size sceneSize = Scene::Size();

    // 左上の攻撃ボタン群と逃げる（右端）
    const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
    const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
    const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
    const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
    const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);

    // 攻撃／逃げる／防御の入力
    const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
    const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
    if (attackBtn1.mouseOver() || attackBtn2.mouseOver() || attackBtn3.mouseOver() || attackBtn4.mouseOver() || escapeBtn.mouseOver() || defendBtn.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    // 攻撃可否（防御中・待機中・コスト不足で不可）
    if (attackBtn1.leftClicked())
    {
        const auto& cards = m_deck.current();
        const bool hasCard = (cards.size() > 0);
        if (hasCard)
        {
            const CardSpec& c = cards[0];
            if (BattleLogic::canAttack(m_state) && BattleLogic::trySpendCost(m_state, c.cost))
            {
                BattleLogic::handlePlayerAttack(m_state, BattleUtils::slotDamage(0));
                m_deck.onUse(0);
            }
            else
            {
                m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
                m_state.waitingForAcknowledge = true;
                m_state.nextAction = BattleState::NextAction::BackToSelection;
            }
        }
        else
        {
            m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
    }
    else if (attackBtn2.leftClicked())
    {
        const auto& cards = m_deck.current();
        const bool hasCard = (cards.size() > 1);
        if (hasCard)
        {
            const CardSpec& c = cards[1];
            if (BattleLogic::canAttack(m_state) && BattleLogic::trySpendCost(m_state, c.cost))
            {
                BattleLogic::handlePlayerAttack(m_state, BattleUtils::slotDamage(1));
                m_deck.onUse(1);
            }
            else
            {
                m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
                m_state.waitingForAcknowledge = true;
                m_state.nextAction = BattleState::NextAction::BackToSelection;
            }
        }
        else
        {
            m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
    }
    else if (attackBtn3.leftClicked())
    {
        const auto& cards = m_deck.current();
        const bool hasCard = (cards.size() > 2);
        if (hasCard)
        {
            const CardSpec& c = cards[2];
            if (BattleLogic::canAttack(m_state) && BattleLogic::trySpendCost(m_state, c.cost))
            {
                BattleLogic::handlePlayerAttack(m_state, BattleUtils::slotDamage(2));
                m_deck.onUse(2);
            }
            else
            {
                m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
                m_state.waitingForAcknowledge = true;
                m_state.nextAction = BattleState::NextAction::BackToSelection;
            }
        }
        else
        {
            m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
    }
    else if (attackBtn4.leftClicked())
    {
        const auto& cards = m_deck.current();
        const bool hasCard = (cards.size() > 3);
        if (hasCard)
        {
            const CardSpec& c = cards[3];
            if (BattleLogic::canAttack(m_state) && BattleLogic::trySpendCost(m_state, c.cost))
            {
                BattleLogic::handlePlayerAttack(m_state, BattleUtils::slotDamage(3));
                m_deck.onUse(3);
            }
            else
            {
                m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
                m_state.waitingForAcknowledge = true;
                m_state.nextAction = BattleState::NextAction::BackToSelection;
            }
        }
        else
        {
            m_state.battleMessage = m_state.defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
    }
    else if (escapeBtn.leftClicked())
    {
        // 敗北として終了（メッセージ表示）
        m_state.playerHP = 0;
        m_state.battleMessage = U"プレイヤーは逃げ出した！";
        m_state.waitingForAcknowledge = true;
        m_state.nextAction = BattleState::NextAction::FinishBattle;
    }
    else if (defendBtn.leftClicked())
    {
        if (BattleLogic::canDefend(m_state) && BattleLogic::trySpendCost(m_state, 20))
        {
            m_state.defending = true;
            m_state.defendTimer.restart();
            m_state.battleMessage = U"防御体勢に入った！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
        else if (!m_state.defending)
        {
            m_state.battleMessage = U"コスト不足！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
    }

    // ---- 敵 AI 更新（詠唱・防御・意思決定）----
    if (!m_state.waitingForAcknowledge)
    {
        enemyUpdateAI();
    }
}

void Game::draw() const
{
    const Size sceneSize = Scene::Size();

    if ((not m_sceneRT) || (m_sceneRT.size() != sceneSize))
	{
		const_cast<Game*>(this)->m_sceneRT = RenderTexture{ sceneSize };
	}
	if ((not m_blurInternal) || (m_blurInternal.size() != sceneSize))
	{
		const_cast<Game*>(this)->m_blurInternal = RenderTexture{ sceneSize };
	}
	if ((not m_blurTarget) || (m_blurTarget.size() != sceneSize))
	{
		const_cast<Game*>(this)->m_blurTarget = RenderTexture{ sceneSize };
	}

    // レイアウト計算（ui に委譲）
    const Vec2 playerPos = BattleLayout::PlayerPos(sceneSize);
    const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
    const Size entitySize = BattleLayout::EntitySize;

    const int32 playerHPWidth = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_state.playerHP) / BattleState::MaxHP)));
    const int32 enemyHPWidth  = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_state.enemyHP) / BattleState::MaxHP)));

    // 旧ボタン変数の残存参照を削除済み

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(ColorF{ 1.0 });

        // キャラ矩形
		{
            // 被弾フラッシュ演出
            const double t = m_state.hitTimer.sF();
            const bool hitPlayer = (m_state.hitTarget == BattleState::HitTarget::Player) && (t < BattleState::HitDuration);
            const bool hitEnemy  = (m_state.hitTarget == BattleState::HitTarget::Enemy)  && (t < BattleState::HitDuration);
			const double flash = hitPlayer || hitEnemy ? (0.5 + 0.5 * Periodic::Square0_1(30.0)) : 0.0;
            const ColorF playerColor = hitPlayer ? ColorF{ 1.0, 0.95 * flash, 0.95 * flash } : ColorF{ 1.0 };
            const ColorF enemyColor  = hitEnemy  ? ColorF{ 1.0, 0.85 * flash, 0.85 * flash } : ColorF{ 1.0 };
			const RectF pRect{ playerPos, BattleLayout::EntitySize };
			const RectF eRect{ enemyPos,  BattleLayout::EntitySize };
			drawFit(m_texPlayer, pRect, playerColor);
			drawFit(m_texEnemy,  eRect,  enemyColor);

            // 敵の状態表示（詠唱・防御）
            {
                const Vec2 infoPos = enemyPos + Vec2{ entitySize.x * 0.5, -12 };
                if (m_enemyCasting)
                {
                    const double p = Clamp(m_enemyCastTimeSec > 0.0 ? (m_enemyCastTimer.sF() / m_enemyCastTimeSec) : 0.0, 0.0, 1.0);
                    const double w = entitySize.x;
                    const RectF barBG{ enemyPos.x, enemyPos.y - 18, w, 6 };
                    const RectF barFG{ barBG.x, barBG.y, w * p, 6 };
                    barBG.draw(ColorF{ 0.2, 0.2, 0.3 });
                    barFG.draw(ColorF{ 1.0, 0.5, 0.2 });
                    FontAsset(U"Bold")(U"詠唱中: {}"_fmt(m_enemyDisplayedLabel)).draw(14, infoPos.movedBy(-entitySize.x * 0.5, -16), ColorF{ 0.95 });
                }
                else if (m_enemyDefending)
                {
                    FontAsset(U"Bold")(U"防御中").draw(14, infoPos.movedBy(-28, -12), ColorF{ 0.95 });
                }
            }
		}

		const Font& bold = FontAsset(U"Bold");

        // HPバー（自分・頭上）
        const RectF playerHPBar = BattleLayout::PlayerHPBarBG(sceneSize);
        const ColorF playerHPColor = BattleUtils::hpColor(m_state.playerHP, BattleState::MaxHP);
        playerHPBar.draw(ColorF{ 0.2 });
        RectF{ playerHPBar.x, playerHPBar.y, static_cast<double>(playerHPWidth), BattleLayout::HPBarHeight }.draw(playerHPColor);
        bold(U"HP {}/{}"_fmt(m_state.playerHP, BattleState::MaxHP)).draw(16, BattleLayout::PlayerHPLabelPos(sceneSize), ColorF{ 0.95 });

        // HPバー（敵・頭上）
        const RectF enemyHPBar = BattleLayout::EnemyHPBarBG(sceneSize);
        const ColorF enemyHPColor = BattleUtils::hpColor(m_state.enemyHP, BattleState::MaxHP);
        enemyHPBar.draw(ColorF{ 0.2 });
        RectF{ enemyHPBar.x, enemyHPBar.y, static_cast<double>(enemyHPWidth), BattleLayout::HPBarHeight }.draw(enemyHPColor);
        bold(U"HP {}/{}"_fmt(m_state.enemyHP, BattleState::MaxHP)).draw(16, BattleLayout::EnemyHPLabelPos(sceneSize), ColorF{ 0.95 });

        // クレイジーゲージ（プレイヤー）
        {
            const Vec2 c = BattleLayout::PlayerCrazyCenter(sceneSize);
            const double ratio = Clamp(m_state.playerCrazy / 100.0, 0.0, 1.0);
            Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(6, 0, ColorF{ 0.85 });
            const double angle = Math::TwoPiF * ratio;
            Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 0.2, 0.6, 1.0 });
            // 顔テクスチャ
            const s3d::Texture& face = m_faces.select(m_state.playerCrazy);
            const double s = 26.0;
            face.scaled(s / face.height()).drawAt(c);
        }

        // クレイジーゲージ（敵）
        {
            const Vec2 c = BattleLayout::EnemyCrazyCenter(sceneSize);
            const double ratio = Clamp(m_state.enemyCrazy / 100.0, 0.0, 1.0);
            Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(6, 0, ColorF{ 0.85 });
            const double angle = Math::TwoPiF * ratio;
            Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 1.0, 0.4, 0.4 });
            const s3d::Texture& face = m_faces.select(m_state.enemyCrazy);
            const double s = 26.0;
            face.scaled(s / face.height()).drawAt(c);
        }

        // 左上の攻撃1〜4（背景色統一）
        const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
        const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
        const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
        const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
        const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);
        const ColorF actionBg{ 1.0 };
        const bool disabledAll = (m_state.waitingForAcknowledge || m_state.defending);

        auto drawSlot = [&](const RoundRect& rr, int slot)
        {
            const auto& cards = m_deck.current();
            const auto& last  = m_deck.lastDisplayed();
            const bool hasCurrent = (slot < static_cast<int>(cards.size()));
            const bool hasLast = (!hasCurrent && (slot < static_cast<int>(last.size())));
            const bool hasAny = hasCurrent || hasLast;
            const ColorF base = disabledAll || !hasAny ? ColorF{ 0.95 } : actionBg;
            rr.draw(base).drawFrame(2);
			String title;
			if (hasAny)
			{
                const CardSpec& c = hasCurrent ? cards[slot] : last[slot];
				title = (c.name.isEmpty() ? U"攻撃{}"_fmt(slot + 1) : c.name);
			}
			else
			{
				title = U"攻撃{}"_fmt(slot + 1);
			}
            const ColorF txt = disabledAll || !hasAny ? ColorF{ 0.5 } : ColorF{ 0.1 };
            bold(title).drawAt(20, rr.center(), txt);
        };

        drawSlot(attackBtn1, 0);
        drawSlot(attackBtn2, 1);
        drawSlot(attackBtn3, 2);
        drawSlot(attackBtn4, 3);
        
        // 逃げる（右端）
		escapeBtn.draw(ColorF{ 1.0, m_escapeTr.value() }); BaseFrame().draw(escapeBtn.rect);
        bold(U"逃げる").drawAt(24, escapeBtn.center(), ColorF{ 0.1 });

        // 左上：コストボックス
        const RectF costPanel = BattleLayout::CostPanelRect(sceneSize);
        const RoundRect costRR{ costPanel, BattleLayout::CostPanelR };
        costRR.draw(ColorF{ 1.0, 0.95 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });
        const int32 cost = m_state.cost();
        
        const double w = costPanel.w - 24;
        const RectF barBG{ costPanel.x + 12, costPanel.y + costPanel.h - 22, w, 10 };
        const RectF barFG{ barBG.x, barBG.y, w * (cost / 100.0), barBG.h };
        const bool blocked = BattleLogic::isRegenBlocked(m_state);
        barBG.draw(ColorF{ 0.85 });
        barFG.draw(blocked ? ColorF{ 0.6 } : ColorF{ 0.2, 0.6, 1.0 });
        FontAsset(U"Bold")(U"COST {}/100"_fmt(cost)).draw(20, Vec2{ costPanel.x + 12, costPanel.y + 10 }, ColorF{ 0.1 });

        // デバッグ表示は削除済み

        // 左下プレイヤーパネル
        const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
        const RoundRect panelRR{ playerPanel, BattleLayout::PlayerPanelR };
		panelRR.draw(ColorF{ 1.0, 0.95 }); BaseFrame().draw(panelRR.rect);
        BattleLayout::PlayerIconRect(playerPanel).rounded(6).draw(ColorF{ 0.3, 0.7, 0.9 });
        const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
        defendBtn.draw(ColorF{ 1.0 }).drawFrame(2);
        bold(U"防御").drawAt(24, defendBtn.center(), ColorF{ 0.1 });


		// メッセージウィンドウ
        if (m_state.waitingForAcknowledge)
		{
			const double panelW = sceneSize.x - 40;
			const double panelH = 110;
			const double panelX = (sceneSize.x - panelW) / 2.0;
			const double panelY = sceneSize.y - 8 - panelH;
			const RoundRect msgPanel{ RectF{ panelX, panelY, panelW, panelH }, 8 };
			msgPanel.draw(ColorF{ 0.95, 0.95, 0.96, 0.94 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });
            FontAsset(U"Bold")(m_state.battleMessage).draw(24, Vec2{ msgPanel.rect.x + 20, msgPanel.rect.y + 20 }, ColorF{ 0.1 });
			FontAsset(U"Bold")(U"キー入力で進む").draw(18, Vec2{ msgPanel.rect.x + 20, msgPanel.rect.y + 64 }, ColorF{ 0.2 });
		}
	}

    if (m_paused)
    {
        Shader::GaussianBlur(m_sceneRT, m_blurInternal, m_blurTarget, BoxFilterSize::BoxFilter9x9);
        m_blurTarget.draw();
        Rect{ sceneSize }.draw(PauseTheme::Dimmer);
        m_pauseMenu.draw();
        Cursor::RequestStyle(CursorStyle::Default);
    }
	else
	{
		m_sceneRT.draw();
		Cursor::RequestStyle(CursorStyle::Default);
	}
	ScreenFrame().draw(s3d::RectF{ 0, 0, (double)Scene::Width(), (double)Scene::Height() });
}
void Game::finishBattleIfNeeded()
{
    if ((m_state.playerHP <= 0) || (m_state.enemyHP <= 0))
	{
		getData().lastMode = GameData::GameMode::PvE;
        getData().lastScore = Max(0, m_state.playerHP);
		changeScene(State::Result);
	}
}


// =========================
// 敵 AI 実装
// =========================
void Game::enemyRegenCost(double dt)
{
    m_enemyCostValue = Min(100.0, m_enemyCostValue + (EnemyCostRegenPerSec * dt));
}

bool Game::enemyIsRegenBlocked() const
{
    return (m_enemyDefending || m_state.waitingForAcknowledge);
}

bool Game::enemyTrySpendCost(int32 amount)
{
    if (enemyCost() < amount)
    {
        return false;
    }
    m_enemyCostValue = Max(0.0, m_enemyCostValue - amount);
    return true;
}

void Game::enemyStartDefend()
{
    if (m_enemyDefending)
        return;
    if (!enemyTrySpendCost(20))
        return;
    m_enemyDefending = true;
    m_enemyDefendTimer.restart();
    // 軽いメッセージを表示（進行一時停止）
    m_state.battleMessage = U"敵は防御体勢に入った！";
    m_state.waitingForAcknowledge = true;
    m_state.nextAction = BattleState::NextAction::BackToSelection;
}

void Game::enemyStartCastAttack(int32 damage, double castSec, const String& label, const String& displayLabel)
{
    if (m_enemyCasting)
        return;
    // 攻撃コストは仮に 10
    if (!enemyTrySpendCost(10))
        return;
    m_enemyCasting = true;
    m_enemyPlannedDamage = Max(0, damage);
    m_enemyCastTimeSec = Max(0.1, castSec);
    m_enemyPlannedLabel = label;
    m_enemyDisplayedLabel = displayLabel;
    m_enemyCastTimer.restart();
}

void Game::enemyResolveCast()
{
    m_enemyCasting = false;
    const bool playerBlocked = m_state.defending;
    const int32 dealt = playerBlocked ? 0 : m_enemyPlannedDamage;
    m_state.playerHP = Max(0, m_state.playerHP - dealt);
    // 敵の攻撃でプレイヤーのクレイジーが増加
    BattleLogic::addCrazy(m_state, false, +20);
    // 敵はクレイジーを少し発散
    BattleLogic::addCrazy(m_state, true, -30);
    BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Player);
    m_state.battleMessage = (dealt == 0)
        ? U"敵の{}は防がれた！0のダメージ！"_fmt(m_enemyPlannedLabel)
        : U"敵は{}を発動！{}のダメージ！"_fmt(m_enemyPlannedLabel, dealt);
    m_state.waitingForAcknowledge = true;
    m_state.nextAction = BattleState::NextAction::BackToSelection;
}

void Game::enemyUpdateAI()
{
    // 防御の継続時間
    if (m_enemyDefending && (m_enemyDefendTimer.sF() >= BattleState::DefendDurationSec))
    {
        m_enemyDefending = false;
    }

    // 詠唱中の進行
    if (m_enemyCasting)
    {
        if (m_enemyCastTimer.sF() >= m_enemyCastTimeSec)
        {
            enemyResolveCast();
        }
        return; // 詠唱中は新規行動しない
    }

    // 意思決定（簡易ルール）
    // 低 HP かつコスト充分なら防御優先
    if (!m_enemyDefending && (m_state.enemyHP <= 25) && enemyCost() >= 20)
    {
        enemyStartDefend();
        return;
    }

    // 攻撃：コスト充分、非防御時
    if (!m_enemyDefending && enemyCost() >= 10)
    {
        // ダメージと詠唱時間をクレイジーや乱数で決定
        int32 dmg = 0;
        if (m_state.enemyCrazy < 60)
        {
            dmg = Random(8, 16);
        }
        else if (m_state.enemyCrazy < 100)
        {
            dmg = Random(12, 22);
        }
        else
        {
            // クレイジー状態：よりハイリスク/ハイリターン
            dmg = Random(6, 28);
        }
        const double castSec = Random(0.5, 1.4);

        // ラベル（実際と表示）。クレイジー時はあべこべ表示
        const String realLabel = U"攻撃";
        String displayLabel = realLabel;
        if (enemyInCrazy())
        {
            // 表示は偽装（例：防御っぽく見せる）
            displayLabel = FakeActionLabels.choice();
        }

        enemyStartCastAttack(dmg, castSec, realLabel, displayLabel);
        return;
    }
}

