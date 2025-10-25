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

    // ボタン領域定義（下部）
    const RoundRect attackBtn = BattleLayout::AttackMainButton(sceneSize);
    const RoundRect escapeBtn = BattleLayout::EscapeMainButton(sceneSize);
    const RoundRect attack1Btn = BattleLayout::Attack1Button(sceneSize);
    const RoundRect attack2Btn = BattleLayout::Attack2Button(sceneSize);

	if (m_showAttackOptions)
	{
		m_attack1Tr.update(attack1Btn.mouseOver());
		m_attack2Tr.update(attack2Btn.mouseOver());
		if (attack1Btn.mouseOver() || attack2Btn.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (attack1Btn.leftClicked())
		{
			handlePlayerAttack(Damage1);
			m_showAttackOptions = false;
		}
		else if (attack2Btn.leftClicked())
		{
			handlePlayerAttack(Damage2);
			m_showAttackOptions = false;
		}
	}
	else
	{
		m_attackTr.update(attackBtn.mouseOver());
		m_escapeTr.update(escapeBtn.mouseOver());
		if (attackBtn.mouseOver() || escapeBtn.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (attackBtn.leftClicked())
		{
			m_showAttackOptions = true;
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

    const RoundRect attackBtn = BattleLayout::AttackMainButton(sceneSize);
    const RoundRect escapeBtn = BattleLayout::EscapeMainButton(sceneSize);
    const RoundRect attack1Btn = BattleLayout::Attack1Button(sceneSize);
    const RoundRect attack2Btn = BattleLayout::Attack2Button(sceneSize);

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

        // HPバー（自分）
        BattleLayout::PlayerHPBarBG(sceneSize).draw(ColorF{ 0.2 });
        RectF{ playerPos.x, playerPos.y + entitySize.y + BattleLayout::HPBarYOffset, static_cast<double>(playerHPWidth), BattleLayout::HPBarHeight }.draw(ColorF{ 0.2, 0.8, 0.3 });
        bold(U"HP {}/{}"_fmt(m_playerHP, MaxHP)).draw(16, BattleLayout::PlayerHPLabelPos(sceneSize), ColorF{ 0.95 });

        // HPバー（敵）
        BattleLayout::EnemyHPBarBG(sceneSize).draw(ColorF{ 0.2 });
        RectF{ enemyPos.x, enemyPos.y + entitySize.y + BattleLayout::HPBarYOffset, static_cast<double>(enemyHPWidth), BattleLayout::HPBarHeight }.draw(ColorF{ 0.9, 0.3, 0.3 });
        bold(U"HP {}/{}"_fmt(m_enemyHP, MaxHP)).draw(16, BattleLayout::EnemyHPLabelPos(sceneSize), ColorF{ 0.95 });

		// ボタン
		if (m_showAttackOptions)
		{
			attack1Btn.draw(ColorF{ 1.0, m_attack1Tr.value() }).drawFrame(2);
			attack2Btn.draw(ColorF{ 1.0, m_attack2Tr.value() }).drawFrame(2);
			bold(U"攻撃1").drawAt(24, attack1Btn.center(), ColorF{ 0.1 });
			bold(U"攻撃2").drawAt(24, attack2Btn.center(), ColorF{ 0.1 });
		}
		else
		{
			attackBtn.draw(ColorF{ 1.0, m_attackTr.value() }).drawFrame(2);
			escapeBtn.draw(ColorF{ 1.0, m_escapeTr.value() }).drawFrame(2);
			bold(U"攻撃する").drawAt(24, attackBtn.center(), ColorF{ 0.1 });
			bold(U"逃げる").drawAt(24, escapeBtn.center(), ColorF{ 0.1 });
		}

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


