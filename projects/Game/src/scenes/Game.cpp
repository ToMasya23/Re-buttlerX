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
		m_resumeTr.update(m_resumeButton.mouseOver());
		m_settingsTr.update(m_settingsButton.mouseOver());
		m_howToTr.update(m_howToButton.mouseOver());
		m_effectTr.update(m_effectButton.mouseOver());
		m_titleTr.update(m_titleButton.mouseOver());
		m_exitTr.update(m_exitButton.mouseOver());

		if (m_resumeButton.mouseOver() || m_settingsButton.mouseOver() || m_howToButton.mouseOver()
			|| m_effectButton.mouseOver() || m_titleButton.mouseOver() || m_exitButton.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (m_resumeButton.leftClicked())
		{
			m_paused = false;
		}
		else if (m_settingsButton.leftClicked())
		{
			changeScene(State::Settings);
		}
		else if (m_howToButton.leftClicked())
		{
			changeScene(State::HowToPlay);
		}
		else if (m_effectButton.leftClicked())
		{
			changeScene(State::EffectViewer);
		}
		else if (m_titleButton.leftClicked())
		{
			changeScene(State::Title);
		}
		else if (m_exitButton.leftClicked())
		{
			System::Exit();
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
			// 敗北として終了
			m_playerHP = 0;
			finishBattleIfNeeded();
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
		RectF(playerPos, entitySize).rounded(6).draw(ColorF{ 0.3, 0.7, 0.9 });
		RectF(enemyPos, entitySize).rounded(6).draw(ColorF{ 0.9, 0.4, 0.4 });

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
	}

	if (m_paused)
	{
		Shader::GaussianBlur(m_sceneRT, m_blurInternal, m_blurTarget, BoxFilterSize::BoxFilter9x9);
		m_blurTarget.draw();
		Rect{ sceneSize }.draw(PauseTheme::Dimmer);

		const Font& title = FontAsset(U"TitleFont");
		const Font& bold = FontAsset(U"Bold");
		const RoundRect panel{ Arg::center(PauseTheme::PanelCenter), PauseTheme::PanelSize, PauseTheme::PanelR };
		panel.draw(PauseTheme::PanelFill).drawFrame(3, 0, PauseTheme::PanelFrame);
		title(U"PAUSE").drawAt(64, Vec2{ PauseTheme::TitlePos }, PauseTheme::TitleColor);

		m_resumeButton.draw(ColorF{ 1.0, m_resumeTr.value() }).drawFrame(2);
		m_settingsButton.draw(ColorF{ 1.0, m_settingsTr.value() }).drawFrame(2);
		m_howToButton.draw(ColorF{ 1.0, m_howToTr.value() }).drawFrame(2);
		m_effectButton.draw(ColorF{ 1.0, m_effectTr.value() }).drawFrame(2);
		m_titleButton.draw(ColorF{ 1.0, m_titleTr.value() }).drawFrame(2);
		m_exitButton.draw(ColorF{ 1.0, m_exitTr.value() }).drawFrame(2);

		bold(U"再開").drawAt(28, m_resumeButton.center(), ColorF{ 0.1 });
		bold(U"設定").drawAt(28, m_settingsButton.center(), ColorF{ 0.1 });
		bold(U"ゲーム説明").drawAt(28, m_howToButton.center(), ColorF{ 0.1 });
		bold(U"効果確認").drawAt(28, m_effectButton.center(), ColorF{ 0.1 });
		bold(U"タイトルへ").drawAt(28, m_titleButton.center(), ColorF{ 0.1 });
		bold(U"EXIT").drawAt(28, m_exitButton.center(), ColorF{ 0.1 });

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
	m_enemyHP = Max(0, m_enemyHP - damage);
	if (m_enemyHP <= 0)
	{
		finishBattleIfNeeded();
		return;
	}

	// 反撃
	enemyCounterAttack();
}

void Game::enemyCounterAttack()
{
	const int32 enemyDamage = Random(8, 16);
	m_playerHP = Max(0, m_playerHP - enemyDamage);
	finishBattleIfNeeded();
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


