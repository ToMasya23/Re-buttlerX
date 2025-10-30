# include "Game.hpp"
# include "../tools/NineSlice.hpp"
namespace
{
	static constexpr int32 Damage1 = 10;
	static constexpr int32 Damage2 = 20;

	NineSliceSkin& ScreenFrame() {
		static NineSliceSkin skin{
			U"assets/ui/frames/battle_frame.png",
			20, 20, 20, 20,   // left, right, top, bottom（像素）
			false             // 中心不画，只画边
		};
		return skin;
	}

	NineSliceSkin& BaseFrame() {
		static NineSliceSkin skin{
			U"assets/ui/frames/battle_base.png",
			20, 20, 20, 20,   // left, right, top, bottom（像素）
			false             // center不绘制
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
    // 顔テクスチャをロード（素材は assets/ui/faces/ 配下）
    m_texSmile = s3d::Texture{ U"assets/ui/faces/笑顔CG一.png" };
    m_texMagao = s3d::Texture{ U"assets/ui/faces/真顔CG二.png" };
    m_texCloudy = s3d::Texture{ U"assets/ui/faces/怪しめCG三.png" };
    m_texCrying = s3d::Texture{ U"assets/ui/faces/泣きCG四.png" };

	m_texPlayer = s3d::Texture{ U"assets/ui/characters/player.png", s3d::TextureDesc::Unmipped };
	m_texEnemy = s3d::Texture{ U"assets/ui/characters/enemy.png",  s3d::TextureDesc::Unmipped };

	m_texDfend_on = s3d::Texture{ U"assets/ui/defend_on.png", s3d::TextureDesc::Unmipped };
	m_texDfend_off = s3d::Texture{ U"assets/ui/defend_off.png",  s3d::TextureDesc::Unmipped };
}

const s3d::Texture& Game::selectFaceTexture(int crazyPercent) const
{
    if (crazyPercent < 40)
    {
        return m_texSmile; // 笑顔
    }
    else if (crazyPercent < 60)
    {
        return m_texMagao; // 真顔
    }
    else if (crazyPercent < 80)
    {
        return m_texCloudy; // 怪しい
    }
    else
    {
        return m_texCrying; // 泣き
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
    if (!isRegenBlocked())
    {
        regenCost(Scene::DeltaTime());
    }

    // 防御の継続時間チェック
    if (m_defending && (m_defendTimer.sF() >= DefendDurationSec))
    {
        m_defending = false;
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

    // 攻撃／逃げる／防御の入力
    m_attack1Tr.update(attackBtn1.mouseOver());
    m_attack2Tr.update(attackBtn2.mouseOver());
    const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
    const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
    if (attackBtn1.mouseOver() || attackBtn2.mouseOver() || attackBtn3.mouseOver() || attackBtn4.mouseOver() || escapeBtn.mouseOver() || defendBtn.mouseOver())
    {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    // 攻撃可否（防御中・待機中・コスト不足で不可）
    if (attackBtn1.leftClicked())
    {
        if (canAttack(U"攻撃1") && trySpendCost(10))
            handlePlayerAttack(Damage1);
        else
        {
            m_battleMessage = m_defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_waitingForAcknowledge = true;
            m_nextAction = NextAction::BackToSelection;
        }
    }
    else if (attackBtn2.leftClicked())
    {
        if (canAttack(U"攻撃2") && trySpendCost(10))
            handlePlayerAttack(Damage2);
        else
        {
            m_battleMessage = m_defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_waitingForAcknowledge = true;
            m_nextAction = NextAction::BackToSelection;
        }
    }
    else if (attackBtn3.leftClicked())
    {
        if (canAttack(U"攻撃3") && trySpendCost(10))
            handlePlayerAttack(Damage1 + 5);
        else
        {
            m_battleMessage = m_defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_waitingForAcknowledge = true;
            m_nextAction = NextAction::BackToSelection;
        }
    }
    else if (attackBtn4.leftClicked())
    {
        if (canAttack(U"攻撃4") && trySpendCost(10))
            handlePlayerAttack(Damage2 + 10);
        else
        {
            m_battleMessage = m_defending ? U"防御中は攻撃できない！" : U"コスト不足！";
            m_waitingForAcknowledge = true;
            m_nextAction = NextAction::BackToSelection;
        }
    }
    else if (escapeBtn.leftClicked())
    {
        // 敗北として終了（メッセージ表示）
        m_playerHP = 0;
        m_battleMessage = U"プレイヤーは逃げ出した！";
        m_waitingForAcknowledge = true;
        m_nextAction = NextAction::FinishBattle;
    }
    else if (defendBtn.leftClicked())
    {
        if (canDefend() && trySpendCost(20))
        {
            m_defending = true;
            m_defendTimer.restart();
            m_battleMessage = U"防御体勢に入った！";
            m_waitingForAcknowledge = true;
            m_nextAction = NextAction::BackToSelection;
        }
        else if (!m_defending)
        {
            m_battleMessage = U"コスト不足！";
            m_waitingForAcknowledge = true;
            m_nextAction = NextAction::BackToSelection;
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

    // 旧ボタン変数の残存参照を削除済み

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(ColorF{ 1.0 });

		// キャラ矩形
		{
			// 被弾フラッシュ演出
			const double t = m_hitTimer.sF();
			const bool hitPlayer = (m_hitTarget == HitTarget::Player) && (t < HitDuration);
			const bool hitEnemy  = (m_hitTarget == HitTarget::Enemy)  && (t < HitDuration);
			const double flash = hitPlayer || hitEnemy ? (0.5 + 0.5 * Periodic::Square0_1(30.0)) : 0.0;
			const ColorF playerColor = hitPlayer ? ColorF{ 1.0, 0.95 * flash, 0.95 * flash } : ColorF{ 1.0 };
			const ColorF enemyColor  = hitEnemy  ? ColorF{ 1.0, 0.85 * flash, 0.85 * flash } : ColorF{ 1.0 };
			//RectF(playerPos, entitySize).rounded(6).draw(playerColor);
			//RectF(enemyPos, entitySize).rounded(6).draw(enemyColor);

			// 保持像素风（最近邻）
			const s3d::ScopedRenderStates2D _nn{ s3d::SamplerState::ClampNearest };

			auto drawFit = [](const Texture& tex, const RectF& dst, const ColorF& col = Palette::White)
				{
					const ScopedRenderStates2D _nn{ SamplerState::ClampNearest }; // 像素风更清晰
					const double sx = dst.w / tex.width();
					const double sy = dst.h / tex.height();
					const double s = Min(sx, sy);                 // 关键：取较小值
					const Vec2   size = Vec2{ tex.width(), tex.height() } *s;
					const Vec2   pos = dst.center() - size * 0.5; // 居中
					tex.scaled(s).draw(pos, col);
				};

			// 依据目标尺寸拉伸到和原来矩形一样大（以左上角 playerPos / enemyPos 为基准）
			const RectF pRect{ playerPos, BattleLayout::EntitySize };
			const RectF eRect{ enemyPos,  BattleLayout::EntitySize };
			drawFit(m_texPlayer, pRect, playerColor);
			drawFit(m_texEnemy, eRect, enemyColor);
		}

		const Font& bold = FontAsset(U"Bold");

        // HPバー（自分・頭上）
        const RectF playerHPBar = BattleLayout::PlayerHPBarBG(sceneSize);
        const ColorF playerHPColor = hpColor(m_playerHP, MaxHP);
        playerHPBar.draw(ColorF{ 0.2 });
        RectF{ playerHPBar.x, playerHPBar.y, static_cast<double>(playerHPWidth), BattleLayout::HPBarHeight }.draw(playerHPColor);
        bold(U"HP {}/{}"_fmt(m_playerHP, MaxHP)).draw(16, BattleLayout::PlayerHPLabelPos(sceneSize), ColorF{ Palette::Pink });

        // HPバー（敵・頭上）
        const RectF enemyHPBar = BattleLayout::EnemyHPBarBG(sceneSize);
        const ColorF enemyHPColor = hpColor(m_enemyHP, MaxHP);
        enemyHPBar.draw(ColorF{ 0.2 });
        RectF{ enemyHPBar.x, enemyHPBar.y, static_cast<double>(enemyHPWidth), BattleLayout::HPBarHeight }.draw(enemyHPColor);
        bold(U"HP {}/{}"_fmt(m_enemyHP, MaxHP)).draw(16, BattleLayout::EnemyHPLabelPos(sceneSize), ColorF{ Palette::Pink });

        // クレイジーゲージ（プレイヤー）
        {
            const Vec2 c = BattleLayout::PlayerCrazyCenter(sceneSize);
            const double ratio = Clamp(m_playerCrazy / 100.0, 0.0, 1.0);
            Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(6, 0, ColorF{ 0.85 });
            const double angle = Math::TwoPiF * ratio;
            Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 0.2, 0.6, 1.0 });
            // 顔テクスチャ
            const s3d::Texture& face = selectFaceTexture(m_playerCrazy);
            const double s = 26.0;
            face.scaled(s / face.height()).drawAt(c);
        }

        // クレイジーゲージ（敵）
        {
            const Vec2 c = BattleLayout::EnemyCrazyCenter(sceneSize);
            const double ratio = Clamp(m_enemyCrazy / 100.0, 0.0, 1.0);
            Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(6, 0, ColorF{ 0.85 });
            const double angle = Math::TwoPiF * ratio;
            Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 1.0, 0.4, 0.4 });
            const s3d::Texture& face = selectFaceTexture(m_enemyCrazy);
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
		attackBtn1.rect.draw(actionBg); BaseFrame().draw(attackBtn1.rect);
		attackBtn2.rect.draw(actionBg); BaseFrame().draw(attackBtn2.rect);
		attackBtn3.rect.draw(actionBg); BaseFrame().draw(attackBtn3.rect);
		attackBtn4.rect.draw(actionBg); BaseFrame().draw(attackBtn4.rect);
        bold(U"攻撃1").drawAt(24, attackBtn1.center(), ColorF{ 0.1 });
        bold(U"攻撃2").drawAt(24, attackBtn2.center(), ColorF{ 0.1 });
        bold(U"攻撃3").drawAt(24, attackBtn3.center(), ColorF{ 0.1 });
        bold(U"攻撃4").drawAt(24, attackBtn4.center(), ColorF{ 0.1 });
        // 逃げる（右端）
		escapeBtn.draw(ColorF{ 1.0, m_escapeTr.value() }); BaseFrame().draw(escapeBtn.rect);
        bold(U"逃げる").drawAt(24, escapeBtn.center(), ColorF{ 0.1 });

        // 左上：コストボックス
        const RectF costPanel = BattleLayout::CostPanelRect(sceneSize);
        //const RoundRect costRR{ costPanel, BattleLayout::CostPanelR };
        //costRR.draw(ColorF{ 1.0, 0.95 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });

		costPanel.draw(ColorF{ 1.0, 0.95 });
		BaseFrame().draw(costPanel);

        const int32 cost = this->cost();
        const double w = costPanel.w - 24;
        const RectF barBG{ costPanel.x + 12, costPanel.y + costPanel.h - 22, w, 10 };
        const RectF barFG{ barBG.x, barBG.y, w * (cost / 100.0), barBG.h };
        const bool blocked = isRegenBlocked();
        barBG.draw(ColorF{ 0.85 });
        barFG.draw(blocked ? ColorF{ 0.6 } : ColorF{ 0.2, 0.6, 1.0 });
        FontAsset(U"Bold")(U"COST {}/100"_fmt(cost)).draw(20, Vec2{ costPanel.x + 12, costPanel.y + 10 }, ColorF{ 0.1 });

        // 左下プレイヤーパネル
        const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
        const RoundRect panelRR{ playerPanel, BattleLayout::PlayerPanelR };
		panelRR.draw(ColorF{ 1.0, 0.95 }); BaseFrame().draw(panelRR.rect);
        BattleLayout::PlayerIconRect(playerPanel).rounded(6).draw(ColorF{ 0.3, 0.7, 0.9 });
        //const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
        //defendBtn.draw(ColorF{ 1.0 }).drawFrame(2);
        //bold(U"防御").drawAt(24, defendBtn.center(), ColorF{ 0.1 });

		const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);

		// （可选）简单底色，保留即可；不要九宫格
		defendBtn.rect.draw(s3d::ColorF{ 1.0 });

		// hover 小高亮（可选）
		if (defendBtn.mouseOver()) {
			defendBtn.rect.draw(s3d::ColorF{ 1.0, 0.06 });
		}

		// 根据状态选择贴图并绘制
		const s3d::Texture& tex = (m_defending ? m_texDfend_on : m_texDfend_off);
		drawFit(tex, defendBtn.rect);

		const RectF dRect{ playerPos, BattleLayout::EntitySize };


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
	ScreenFrame().draw(s3d::RectF{ 0, 0, (double)Scene::Width(), (double)Scene::Height() });
}

void Game::handlePlayerAttack(int32 damage)
{
	// ダメージ適用と演出開始
	m_enemyHP = Max(0, m_enemyHP - damage);
	startHitEffect(HitTarget::Enemy);
	m_battleMessage = U"プレイヤーは敵に攻撃した！{}のダメージを与えた！"_fmt(damage);
	m_waitingForAcknowledge = true;

	// クレイジーゲージ効果
	addCrazy(true, +20);   // 敵を増やす
	addCrazy(false, -10);  // 自分は減らす（バフ）

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
	int32 finalDamage = enemyDamage;
	if (m_defending)
	{
		finalDamage = 0;
	}
	m_playerHP = Max(0, m_playerHP - finalDamage);
	// 敵の攻撃でプレイヤーのクレイジーが増加
	addCrazy(false, +20);
	startHitEffect(HitTarget::Player);
	m_battleMessage = (finalDamage == 0)
		? U"敵は攻撃したが、防御した！0のダメージ！"
		: U"敵はプレイヤーに攻撃した！{}のダメージを与えた！"_fmt(finalDamage);
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

void Game::regenCost(double dt)
{
	m_costValue = Min(100.0, m_costValue + (CostRegenPerSec * dt));
}

int32 Game::calcAttackCost(const String& label) const
{
	return static_cast<int32>(label.size());
}

bool Game::trySpendCost(int32 amount)
{
	if (cost() < amount)
	{
		return false;
	}
	m_costValue = Max(0.0, m_costValue - amount);
	return true;
}

bool Game::isRegenBlocked() const
{
	return (m_defending || m_waitingForAcknowledge);
}

bool Game::canAttack(const String& label) const
{
    (void)label; // ラベル長は使用しない（コストは呼び出し側で判定）
    if (m_defending || m_waitingForAcknowledge)
    {
        return false;
    }
    return true;
}

bool Game::canDefend() const
{
	if (m_defending || m_waitingForAcknowledge)
	{
		return false;
	}
	return (cost() >= 20);
}

void Game::addCrazy(bool targetIsEnemy, int32 delta)
{
	int32& v = targetIsEnemy ? m_enemyCrazy : m_playerCrazy;
	v = Clamp(v + delta, 0, 100);
}

ColorF Game::hpColor(int hp, int maxHP)
{
	const double r = Clamp(static_cast<double>(hp) / Max(1, maxHP), 0.0, 1.0);
	if (r >= 0.5)
	{
		return ColorF{ 0.2, 0.8, 0.3 }; // 緑
	}
	else if (r >= 0.2)
	{
		return ColorF{ 0.95, 0.85, 0.2 }; // 黄
	}
	else
	{
		return ColorF{ 0.9, 0.3, 0.3 }; // 赤
	}
}


