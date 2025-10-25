# include "Game.hpp"
namespace
{
	static constexpr int32 Damage1 = 10;
	static constexpr int32 Damage2 = 20;
}

Game::Game(const InitData& init)
	: IScene{ init }
{
}

void Game::update()
{
	// Esc でポーズをトグル
	if (KeyEscape.down())
	{
		m_paused = (not m_paused);
		return;
	}

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

	// ---- メッセージ待機中は進行を止める ----
	if (m_waitingForAcknowledge)
	{
		if (advanceInputDown())
		{
			m_waitingForAcknowledge = false;
			if (m_nextAction == NextAction::EnemyCounter)
			{
				doEnemyCounterStep();
			}
			else if (m_nextAction == NextAction::FinishBattle)
			{
				finishBattleIfNeeded();
			}
			else if (m_nextAction == NextAction::BackToSelection)
			{
				// そのまま選択に戻る
			}
			m_nextAction = NextAction::None;
		}
		return;
	}

	// ---- PvE バトル更新 ----
    const Size sceneSize = Scene::Size();

    // 左上の攻撃ボタン群と逃げる（右端）
    const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
    const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
    const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
    const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
    const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);

    // 攻撃／逃げるの入力
    m_attack1Tr.update(attackBtn1.mouseOver());
    m_attack2Tr.update(attackBtn2.mouseOver());
    if (attackBtn1.mouseOver() || attackBtn2.mouseOver() || attackBtn3.mouseOver() || attackBtn4.mouseOver() || escapeBtn.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    if (attackBtn1.leftClicked())
    {
        handlePlayerAttack(Damage1);
    }
    else if (attackBtn2.leftClicked())
    {
        handlePlayerAttack(Damage2);
    }
    else if (attackBtn3.leftClicked())
    {
        handlePlayerAttack(Damage1 + 5);
    }
    else if (attackBtn4.leftClicked())
    {
        handlePlayerAttack(Damage2 + 10);
    }
    else if (escapeBtn.leftClicked())
    {
        // 敗北として終了（メッセージ表示）
        m_playerHP = 0;
        m_battleMessage = U"プレイヤーは逃げ出した！";
        m_waitingForAcknowledge = true;
        m_nextAction = NextAction::FinishBattle;
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

    const int32 playerHPWidth = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_playerHP) / MaxHP)));
    const int32 enemyHPWidth  = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_enemyHP) / MaxHP)));

    // 旧ボタン変数の残存参照を削除済み

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(ColorF{ 0.18, 0.2, 0.24 });

		// キャラ矩形
		{
			// 被弾フラッシュ演出
			const double t = m_hitTimer.sF();
			const bool hitPlayer = (m_hitTarget == HitTarget::Player) && (t < HitDuration);
			const bool hitEnemy  = (m_hitTarget == HitTarget::Enemy)  && (t < HitDuration);
			const double flash = hitPlayer || hitEnemy ? (0.5 + 0.5 * Periodic::Square0_1(30.0)) : 0.0;
			const ColorF playerColor = hitPlayer ? ColorF{ 1.0, 0.95 * flash, 0.95 * flash } : ColorF{ 0.3, 0.7, 0.9 };
			const ColorF enemyColor  = hitEnemy  ? ColorF{ 1.0, 0.85 * flash, 0.85 * flash } : ColorF{ 0.9, 0.4, 0.4 };
			RectF(playerPos, entitySize).rounded(6).draw(playerColor);
			RectF(enemyPos, entitySize).rounded(6).draw(enemyColor);
		}

		const Font& bold = FontAsset(U"Bold");

        // HPバー（自分・頭上）
        BattleLayout::PlayerHPBarBG(sceneSize).draw(ColorF{ 0.2 });
        RectF{ BattleLayout::PlayerHPBarBG(sceneSize).x, BattleLayout::PlayerHPBarBG(sceneSize).y, static_cast<double>(playerHPWidth), BattleLayout::HPBarHeight }.draw(ColorF{ 0.2, 0.8, 0.3 });
        bold(U"HP {}/{}"_fmt(m_playerHP, MaxHP)).draw(16, BattleLayout::PlayerHPLabelPos(sceneSize), ColorF{ 0.95 });

        // HPバー（敵・頭上）
        BattleLayout::EnemyHPBarBG(sceneSize).draw(ColorF{ 0.2 });
        RectF{ BattleLayout::EnemyHPBarBG(sceneSize).x, BattleLayout::EnemyHPBarBG(sceneSize).y, static_cast<double>(enemyHPWidth), BattleLayout::HPBarHeight }.draw(ColorF{ 0.9, 0.3, 0.3 });
        bold(U"HP {}/{}"_fmt(m_enemyHP, MaxHP)).draw(16, BattleLayout::EnemyHPLabelPos(sceneSize), ColorF{ 0.95 });

        // 左上の攻撃1〜4（背景色統一）
        const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
        const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
        const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
        const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
        const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);
        const ColorF actionBg{ 1.0 };
        attackBtn1.draw(actionBg).drawFrame(2);
        attackBtn2.draw(actionBg).drawFrame(2);
        attackBtn3.draw(actionBg).drawFrame(2);
        attackBtn4.draw(actionBg).drawFrame(2);
        bold(U"攻撃1").drawAt(24, attackBtn1.center(), ColorF{ 0.1 });
        bold(U"攻撃2").drawAt(24, attackBtn2.center(), ColorF{ 0.1 });
        bold(U"攻撃3").drawAt(24, attackBtn3.center(), ColorF{ 0.1 });
        bold(U"攻撃4").drawAt(24, attackBtn4.center(), ColorF{ 0.1 });
        // 逃げる（右端）
        escapeBtn.draw(ColorF{ 1.0, m_escapeTr.value() }).drawFrame(2);
        bold(U"逃げる").drawAt(24, escapeBtn.center(), ColorF{ 0.1 });

        // 左上：コストボックス
        const RectF costPanel = BattleLayout::CostPanelRect(sceneSize);
        const RoundRect costRR{ costPanel, BattleLayout::CostPanelR };
        costRR.draw(ColorF{ 1.0, 0.95 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });
        const int32 cost = Clamp(m_cost, 0, 100);
        const double w = costPanel.w - 24;
        const RectF barBG{ costPanel.x + 12, costPanel.y + costPanel.h - 22, w, 10 };
        const RectF barFG{ barBG.x, barBG.y, w * (cost / 100.0), barBG.h };
        barBG.draw(ColorF{ 0.8 });
        barFG.draw(ColorF{ 0.2, 0.6, 1.0 });
        FontAsset(U"Bold")(U"COST {}/100"_fmt(cost)).draw(20, Vec2{ costPanel.x + 12, costPanel.y + 10 }, ColorF{ 0.1 });

        // 左下プレイヤーパネル
        const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
        const RoundRect panelRR{ playerPanel, BattleLayout::PlayerPanelR };
        panelRR.draw(ColorF{ 1.0, 0.95 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });
        BattleLayout::PlayerIconRect(playerPanel).rounded(6).draw(ColorF{ 0.3, 0.7, 0.9 });
        const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
        defendBtn.draw(ColorF{ 1.0 }).drawFrame(2);
        bold(U"防御").drawAt(24, defendBtn.center(), ColorF{ 0.1 });

		// メッセージウィンドウ
		if (m_waitingForAcknowledge)
		{
			const double panelW = sceneSize.x - 40;
			const double panelH = 110;
			const double panelX = (sceneSize.x - panelW) / 2.0;
			const double panelY = sceneSize.y - 8 - panelH;
			const RoundRect msgPanel{ RectF{ panelX, panelY, panelW, panelH }, 8 };
			msgPanel.draw(ColorF{ 0.95, 0.95, 0.96, 0.94 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });
			FontAsset(U"Bold")(m_battleMessage).draw(24, Vec2{ msgPanel.rect.x + 20, msgPanel.rect.y + 20 }, ColorF{ 0.1 });
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
}

void Game::handlePlayerAttack(int32 damage)
{
	// ダメージ適用と演出開始
	m_enemyHP = Max(0, m_enemyHP - damage);
	startHitEffect(HitTarget::Enemy);
	m_battleMessage = U"プレイヤーは敵に攻撃した！{}のダメージを与えた！"_fmt(damage);
	m_waitingForAcknowledge = true;

	// 次のアクション判定
	if (m_enemyHP <= 0)
	{
		m_nextAction = NextAction::FinishBattle;
	}
	else
	{
		m_nextAction = NextAction::EnemyCounter;
	}
}

void Game::doEnemyCounterStep()
{
	const int32 enemyDamage = Random(8, 16);
	m_playerHP = Max(0, m_playerHP - enemyDamage);
	startHitEffect(HitTarget::Player);
	m_battleMessage = U"敵はプレイヤーに攻撃した！{}のダメージを与えた！"_fmt(enemyDamage);
	m_waitingForAcknowledge = true;

	if (m_playerHP <= 0)
	{
		m_nextAction = NextAction::FinishBattle;
	}
	else
	{
		m_nextAction = NextAction::BackToSelection;
	}
}

void Game::finishBattleIfNeeded()
{
	if ((m_playerHP <= 0) || (m_enemyHP <= 0))
	{
		getData().lastMode = GameData::GameMode::PvE;
		getData().lastScore = Max(0, m_playerHP);
		changeScene(State::Result);
	}
}

void Game::startHitEffect(HitTarget target)
{
	m_hitTarget = target;
	m_hitTimer.restart();
}

bool Game::advanceInputDown() const
{
	return (MouseL.down() || KeyEnter.down() || KeySpace.down() || KeyZ.down() || KeyX.down());
}


