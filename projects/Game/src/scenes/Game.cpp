# include "Game.hpp"
# include "../game/BattleLogic.hpp"
# include "../game/BattleUtils.hpp"
# include "../tools/NineSlice.hpp"
namespace
{
	static constexpr int32 Damage1 = 10;
	static constexpr int32 Damage2 = 20;

	NineSliceSkin& ScreenFrame() {
		static NineSliceSkin skin{
			U"assets/ui/frames/battle_frame.png",
			20, 20, 20, 20,
			false
		};
		return skin;
	}

	NineSliceSkin& BaseFrame() {
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
		const s3d::Vec2 size = s3d::Vec2{ tex.width(), tex.height() } *s;
		const s3d::Vec2 pos = dst.center() - size * 0.5;
		tex.scaled(s).draw(pos, tint);
	}
}

Game::Game(const InitData& init)
    : IScene{ init }
{
	// 全モード共通の初期化
    m_faces.load();
    m_deck.loadAll();
    if (m_deck.hasCards())
    {
        m_deck.refillRandom(4);
    }

	// キャラクタテクスチャの読み込み（ドットのにじみを避けるため Unmipped）
	m_texPlayer = s3d::Texture{ U"assets/ui/characters/player.png", s3d::TextureDesc::Unmipped };
	m_texEnemy  = s3d::Texture{ U"assets/ui/characters/enemy.png",  s3d::TextureDesc::Unmipped };
	
	// ===== オンライン対戦の初期化 =====
	if (getData().multiplayer)
	{
		m_multiplayer = getData().multiplayer;
		m_isOnlineMode = true;
		m_isHost = getData().isHost;
		m_isMyTurn = m_isHost;  // ホストが先攻
		
		// 初期状態を相手に送信
		sendGameStateSync();
	}
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

	// ===== オンライン対戦のネットワーク処理 =====
	if (m_isOnlineMode && m_multiplayer)
	{
		m_multiplayer->update();
		handleNetworkMessages();
	}

    // 使用後のクールダウン掃除
    m_deck.cleanupCooldowns();

    // コスト回復（停止条件を考慮）
    if (!BattleLogic::isRegenBlocked(m_state))
    {
        BattleLogic::regenCost(m_state, Scene::DeltaTime());
    }

    // 防御の継続時間チェック
    if (m_state.defending && (m_state.defendTimer.sF() >= BattleState::DefendDurationSec))
    {
        m_state.defending = false;
    }

    // ---- メッセージ待機中は進行を止める ----
    if (m_state.waitingForAcknowledge)
	{
        if (BattleLogic::advanceInputDown())
		{
            m_state.waitingForAcknowledge = false;
            if (m_state.nextAction == BattleState::NextAction::EnemyCounter)
			{
				// PvPモードでは敵の反撃はネットワーク経由で来るのでスキップ
				if (!m_isOnlineMode)
				{
					BattleLogic::enemyCounter(m_state);
				}
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

	// 詠唱は使用しない（即時反映）

    // インターバル再抽選の制御は未使用のため削除

	// ---- PvE バトル更新 ----
    const Size sceneSize = Scene::Size();

    // 左上の攻撃ボタン群と逃げる（右端）
    const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
    const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
    const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
    const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
    const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);

    // 攻撃／逃げる／防御の入力
    // 旧ホバー演出は未使用のため削除（escape のみ継続）
    const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
    const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
    if (attackBtn1.mouseOver() || attackBtn2.mouseOver() || attackBtn3.mouseOver() || attackBtn4.mouseOver() || escapeBtn.mouseOver() || defendBtn.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    // 攻撃可否（防御中・待機中・インターバル中・コスト不足で不可）
    if (attackBtn1.leftClicked())
    {
        const auto& cards = m_deck.current();
        const bool hasCard = (cards.size() > 0);
        if (hasCard)
        {
            const CardSpec& c = cards[0];
            if (BattleLogic::canAttack(m_state) && BattleLogic::trySpendCost(m_state, c.cost))
            {
                const int32 damage = BattleUtils::slotDamage(0);
                
                Console << U"[攻撃1] ダメージ計算: " << damage;
                
                if (m_isOnlineMode)
                {
                    // PvPモード：ダメージを送信（敵のHPは減らさない）
                    PlayerActionMessage msg;
                    msg.type = MessageType::PlayerAction;
                    msg.action = ActionType::Attack1;
                    msg.damage = damage;
                    msg.turnNumber = m_turnNumber;
                    
                    Console << U"[攻撃1送信] damage=" << msg.damage << U" action=" << (int)msg.action;
                    m_multiplayer->send(msg);
                    
                    // 自分の状態（コスト消費など）を同期
                    sendGameStateSync();
                    
                    m_state.battleMessage = Format(U"攻撃！ダメージ {}", damage);
                    m_state.waitingForAcknowledge = true;
                    m_state.nextAction = BattleState::NextAction::BackToSelection;
                }
                else
                {
                    // PvEモード：従来通りローカルで処理
                    BattleLogic::handlePlayerAttack(m_state, damage);
                }
                
                // 使用記録とクールダウン開始
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
                const int32 damage = BattleUtils::slotDamage(1);
                
                if (m_isOnlineMode)
                {
                    PlayerActionMessage msg;
                    msg.type = MessageType::PlayerAction;
                    msg.action = ActionType::Attack2;
                    msg.damage = damage;
                    msg.turnNumber = m_turnNumber;
                    m_multiplayer->send(msg);
                    
                    sendGameStateSync();
                    
                    m_state.battleMessage = Format(U"攻撃！ダメージ ", damage);
                    m_state.waitingForAcknowledge = true;
                    m_state.nextAction = BattleState::NextAction::BackToSelection;
                }
                else
                {
                    BattleLogic::handlePlayerAttack(m_state, damage);
                }
                
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
                const int32 damage = BattleUtils::slotDamage(2);
                
                if (m_isOnlineMode)
                {
                    PlayerActionMessage msg;
                    msg.type = MessageType::PlayerAction;
                    msg.action = ActionType::Attack3;
                    msg.damage = damage;
                    msg.turnNumber = m_turnNumber;
                    m_multiplayer->send(msg);
                    
                    sendGameStateSync();
                    
                    m_state.battleMessage = Format(U"攻撃！ダメージ ", damage);
                    m_state.waitingForAcknowledge = true;
                    m_state.nextAction = BattleState::NextAction::BackToSelection;
                }
                else
                {
                    BattleLogic::handlePlayerAttack(m_state, damage);
                }
                
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
                const int32 damage = BattleUtils::slotDamage(3);
                
                if (m_isOnlineMode)
                {
                    PlayerActionMessage msg;
                    msg.type = MessageType::PlayerAction;
                    msg.action = ActionType::Attack4;
                    msg.damage = damage;
                    msg.turnNumber = m_turnNumber;
                    m_multiplayer->send(msg);
                    
                    sendGameStateSync();
                    
                    m_state.battleMessage = Format(U"攻撃！ダメージ ", damage);
                    m_state.waitingForAcknowledge = true;
                    m_state.nextAction = BattleState::NextAction::BackToSelection;
                }
                else
                {
                    BattleLogic::handlePlayerAttack(m_state, damage);
                }
                
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
            if (m_isOnlineMode)
            {
                sendGameStateSync();
            }
        }
        else if (!m_state.defending)
        {
            m_state.battleMessage = U"コスト不足！";
            m_state.waitingForAcknowledge = true;
            m_state.nextAction = BattleState::NextAction::BackToSelection;
        }
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
				title = (c.name.isEmpty() ? U"攻撃"_fmt(slot + 1) : c.name);
			}
			else
			{
				title = U"攻撃"_fmt(slot + 1);
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
		getData().lastMode = m_isOnlineMode ? GameData::GameMode::PvP : GameData::GameMode::PvE;
		getData().lastScore = Max(0, m_state.playerHP);
		changeScene(State::Result);
	}
}

void Game::handleNetworkMessages()
{
	if (!m_multiplayer)
		return;

	// 全てのメッセージを処理（キューが空になるまで）
	while (true)
	{
		auto msgType = m_multiplayer->peekMessageType();
		if (!msgType)
			break;  // メッセージがなければ終了

		Console << U"[handleNetworkMessages] メッセージタイプ: " << (int)*msgType;

		switch (*msgType)
		{
		case MessageType::PlayerAction:
		{
			auto msg = m_multiplayer->receive<PlayerActionMessage>();
			if (msg)
			{
				// 相手の攻撃を受信（damageフィールドを使用）
				int32 damage = msg->damage;
				
				Console << U"[PlayerAction受信] damage=" << damage << U" 現在のplayerHP=" << m_state.playerHP;
				
				if (damage > 0)
				{
					// 防御中なら半減
					if (m_state.defending)
					{
						damage = damage / 2;
						m_state.battleMessage = U"相手の攻撃！ダメージ " + Format(damage) + U"（防御で半減）";
					}
					else
					{
						m_state.battleMessage = U"相手の攻撃！ダメージ " + Format(damage);
					}
					
					m_state.playerHP -= damage;
					m_state.hitTarget = BattleState::HitTarget::Player;
					m_state.hitTimer.restart();
					
					Console << U"[ダメージ適用後] playerHP=" << m_state.playerHP;
					
					// 被ダメージ後、自分の状態を同期
					sendGameStateSync();
					
					// ゲーム終了チェック
					if (m_state.playerHP <= 0)
					{
						m_state.nextAction = BattleState::NextAction::FinishBattle;
					}
					else
					{
						m_state.nextAction = BattleState::NextAction::EnemyCounter;
					}
					
					m_state.waitingForAcknowledge = true;
				}
			}
		}
		break;

	case MessageType::GameStateSync:
		{
			auto msg = m_multiplayer->receive<GameStateSyncMessage>();
			if (msg)
			{
				// デバッグ出力
				Console << U"[GameStateSync受信] isHost=" << m_isHost 
					  << U" hostHP=" << msg->hostHP 
					  << U" clientHP=" << msg->clientHP
					  << U" 現在のenemyHP=" << m_state.enemyHP;
				
				// 相手（enemy）の状態だけを更新し、自分の状態は更新しない
				// add_networkブランチと同じ実装
				if (m_isHost)
				{
					// ホストの場合：相手がclientなので、client側の情報だけを更新
					m_state.enemyHP = msg->clientHP;
					m_state.enemyCrazy = msg->clientCrazy;
					Console << U"[ホスト] enemyHPを更新: " << m_state.enemyHP;
				}
				else
				{
					// クライアントの場合：相手がhostなので、host側の情報だけを更新
					m_state.enemyHP = msg->hostHP;
					m_state.enemyCrazy = msg->hostCrazy;
					Console << U"[クライアント] enemyHPを更新: " << m_state.enemyHP;
				}
				m_turnNumber = msg->turnNumber;
			}
		}
		break;

	case MessageType::TurnChange:
		{
			auto msg = m_multiplayer->receive<TurnChangeMessage>();
			if (msg)
			{
				m_turnNumber = msg->turnNumber;
				m_isMyTurn = (m_isHost == msg->isHostTurn);
			}
		}
		break;

	case MessageType::BattleEnd:
		{
			auto msg = m_multiplayer->receive<BattleEndMessage>();
			if (msg)
			{
				// バトル終了処理
				getData().lastMode = GameData::GameMode::PvP;
				getData().lastScore = m_isHost ? msg->hostFinalHP : msg->clientFinalHP;
				changeScene(State::Result);
			}
		}
		break;

	default:
		Console << U"[handleNetworkMessages] 未知のメッセージタイプ: " << (int)*msgType;
		// 未知のメッセージは破棄
		m_multiplayer->peekMessageType();  // これではキューから削除できない
		break;
	}
	}  // while終了
}

void Game::sendGameStateSync()
{
	if (!m_multiplayer || !m_isOnlineMode)
		return;

	GameStateSyncMessage msg;
	msg.type = MessageType::GameStateSync;

	// 現在の状態をメッセージに格納（add_networkブランチと同じ構造）
	if (m_isHost)
	{
		// ホストの場合：自分がhost、相手がclient
		msg.hostHP = m_state.playerHP;
		msg.hostCost = m_state.costValue;
		msg.hostDefending = m_state.defending;
		msg.hostDefendTime = m_state.defendTimer.sF();
		msg.hostCrazy = m_state.playerCrazy;

		msg.clientHP = m_state.enemyHP;
		msg.clientCost = 0;  // 相手のコストは不要
		msg.clientDefending = false;
		msg.clientDefendTime = 0;
		msg.clientCrazy = m_state.enemyCrazy;
		
		Console << U"[ホスト送信] playerHP=" << m_state.playerHP << U" enemyHP=" << m_state.enemyHP;
	}
	else
	{
		// クライアントの場合：自分がclient、相手がhost
		msg.hostHP = m_state.enemyHP;
		msg.hostCost = 0;
		msg.hostDefending = false;
		msg.hostDefendTime = 0;
		msg.hostCrazy = m_state.enemyCrazy;

		msg.clientHP = m_state.playerHP;
		msg.clientCost = m_state.costValue;
		msg.clientDefending = m_state.defending;
		msg.clientDefendTime = m_state.defendTimer.sF();
		msg.clientCrazy = m_state.playerCrazy;
		
		Console << U"[クライアント送信] playerHP=" << m_state.playerHP << U" enemyHP=" << m_state.enemyHP;
	}

	msg.isHostTurn = m_isHost ? m_isMyTurn : !m_isMyTurn;
	msg.turnNumber = m_turnNumber;

	m_multiplayer->send(msg);
}

void Game::sendPlayerAction(ActionType action)
{
	if (!m_multiplayer || !m_isOnlineMode)
		return;
	
	PlayerActionMessage msg;
	msg.type = MessageType::PlayerAction;
	msg.action = action;
	msg.turnNumber = m_turnNumber;
	
	m_multiplayer->send(msg);
}
