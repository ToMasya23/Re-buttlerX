#include "Game_Impl.hpp"
#include "../game/BattleLogic.hpp"
#include "../game/BattleUtils.hpp"

// =============================================================================
//  UI描画
// =============================================================================

void Game::draw() const
{
	const Size sceneSize = Scene::Size();

	if ((!m_sceneRT) || (m_sceneRT.size() != sceneSize))
	{
		const_cast<Game*>(this)->m_sceneRT = RenderTexture{ sceneSize };
	}
	if ((!m_blurInternal) || (m_blurInternal.size() != sceneSize))
	{
		const_cast<Game*>(this)->m_blurInternal = RenderTexture{ sceneSize };
	}
	if ((!m_blurTarget) || (m_blurTarget.size() != sceneSize))
	{
		const_cast<Game*>(this)->m_blurTarget = RenderTexture{ sceneSize };
	}

	const Vec2 playerPos = BattleLayout::PlayerPos(sceneSize);
	const Vec2 enemyPos  = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;

	const int32 playerHPWidth = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_state.playerHP) / BattleState::MaxHP)));
	const int32 enemyHPWidth  = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_state.enemyHP) / BattleState::MaxHP)));

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		
		// ===== 背景 =====
		if (m_texBattleBackground)
		{
			m_texBattleBackground
				.resized(sceneSize)
				.draw(0, 0);
		}
		else
		{
			m_sceneRT.clear(ColorF{ 1.0 });
		}

		const double t = m_state.hitTimer.sF();
		const bool hitPlayer = (m_state.hitTarget == BattleState::HitTarget::Player) && (t < BattleState::HitDuration);
		const bool hitEnemy  = (m_state.hitTarget == BattleState::HitTarget::Enemy)  && (t < BattleState::HitDuration);
		const double flash = hitPlayer || hitEnemy ? (0.5 + 0.5 * Periodic::Square0_1(30.0)) : 0.0;
		const ColorF playerColor = hitPlayer ? ColorF{ 1.0, 0.95 * flash, 0.95 * flash } : ColorF{ 1.0 };
		const ColorF enemyColor  = hitEnemy  ? ColorF{ 1.0, 0.85 * flash, 0.85 * flash } : ColorF{ 1.0 };
		const RectF pRect{ playerPos, BattleLayout::EntitySize };
		const RectF eRect{ enemyPos,  BattleLayout::EntitySize };

		// ===== オーラ =====
		if (m_auraRenderer)
		{
			m_auraRenderer->draw(pRect, m_state.playerAttributeId);
			m_auraRenderer->draw(eRect, m_state.enemyAttributeId);
		}

		// ===== プレイヤーキャラクター =====
		if (m_playerAttackAnimActive)
		{
			const s3d::Texture* pAtk1 = &m_texPlayerAttackQuantity1;
			const s3d::Texture* pAtk2 = &m_texPlayerAttackQuantity2;
			if (m_state.playerAttributeId == 2)
			{
				pAtk1 = &m_texPlayerAttackQuality1;
				pAtk2 = &m_texPlayerAttackQuality2;
			}
			else if (m_state.playerAttributeId == 3)
			{
				pAtk1 = &m_texPlayerAttackCounter1;
				pAtk2 = &m_texPlayerAttackCounter2;
			}

			const double elapsed = m_playerAttackAnimTimer.sF();
			if (elapsed < AttackAnimFrame1Duration)
				drawFit(*pAtk1, pRect, playerColor);
			else
				drawFit(*pAtk2, pRect, playerColor);
		}
		else
		{
			const s3d::Texture* pIdle = &m_texPlayer;
			if      (m_state.playerAttributeId == 1 && m_texPlayerIdleQuantity) pIdle = &m_texPlayerIdleQuantity;
			else if (m_state.playerAttributeId == 2 && m_texPlayerIdleQuality)  pIdle = &m_texPlayerIdleQuality;
			else if (m_state.playerAttributeId == 3 && m_texPlayerIdleCounter)  pIdle = &m_texPlayerIdleCounter;
			drawFit(*pIdle, pRect, playerColor);
		}

		// ===== 敵キャラクター =====
		if (m_enemyAttackAnimActive)
		{
			const s3d::Texture* pEAtk1 = &m_texEnemyAttackQuantity1;
			const s3d::Texture* pEAtk2 = &m_texEnemyAttackQuantity2;
			if (m_state.enemyAttributeId == 2)
			{
				pEAtk1 = &m_texEnemyAttackQuality1;
				pEAtk2 = &m_texEnemyAttackQuality2;
			}
			else if (m_state.enemyAttributeId == 3)
			{
				pEAtk1 = &m_texEnemyAttackCounter1;
				pEAtk2 = &m_texEnemyAttackCounter2;
			}

			const double elapsed = m_enemyAttackAnimTimer.sF();
			if (elapsed < AttackAnimFrame1Duration)
				drawFit(*pEAtk1, eRect, enemyColor);
			else
				drawFit(*pEAtk2, eRect, enemyColor);
		}
		else
		{
			const s3d::Texture* pEIdle = &m_texEnemy;
			if      (m_state.enemyAttributeId == 1 && m_texEnemyIdleQuantity) pEIdle = &m_texEnemyIdleQuantity;
			else if (m_state.enemyAttributeId == 2 && m_texEnemyIdleQuality)  pEIdle = &m_texEnemyIdleQuality;
			else if (m_state.enemyAttributeId == 3 && m_texEnemyIdleCounter)  pEIdle = &m_texEnemyIdleCounter;
			drawFit(*pEIdle, eRect, enemyColor);
		}

		// ===== HP バー =====
		const Font& bold = FontAsset(U"Bold");

		const RectF playerHPBar = BattleLayout::PlayerHPBarBG(sceneSize);
		const ColorF playerHPColor = BattleUtils::hpColor(m_state.playerHP, BattleState::MaxHP);
		playerHPBar.draw(ColorF{ 0.2 });
		RectF{ playerHPBar.x, playerHPBar.y, static_cast<double>(playerHPWidth), BattleLayout::HPBarHeight }.draw(playerHPColor);
		bold(U"HP {}/{}"_fmt(m_state.playerHP, BattleState::MaxHP)).draw(16, BattleLayout::PlayerHPLabelPos(sceneSize), ColorF{ 0.95 });

		const RectF enemyHPBar = BattleLayout::EnemyHPBarBG(sceneSize);
		const ColorF enemyHPColor = BattleUtils::hpColor(m_state.enemyHP, BattleState::MaxHP);
		enemyHPBar.draw(ColorF{ 0.2 });
		RectF{ enemyHPBar.x, enemyHPBar.y, static_cast<double>(enemyHPWidth), BattleLayout::HPBarHeight }.draw(enemyHPColor);
		bold(U"HP {}/{}"_fmt(m_state.enemyHP, BattleState::MaxHP)).draw(16, BattleLayout::EnemyHPLabelPos(sceneSize), ColorF{ 0.95 });

		// ===== クレイジーメーター =====
		{
			const Vec2 c = BattleLayout::PlayerCrazyCenter(sceneSize);
			const double ratio = Clamp(m_state.playerCrazy / 100.0, 0.0, 1.0);
			Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(12, 0, ColorF{ 0.85 });
			const double angle = Math::TwoPiF * ratio;
			Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 0.2, 0.6, 1.0 });
			const s3d::Texture& face = m_faces.select(m_state.playerCrazy);
			const double s = 70.0;
			face.scaled(s / face.height()).drawAt(c);
		}
		{
			const Vec2 c = BattleLayout::EnemyCrazyCenter(sceneSize);
			const double ratio = Clamp(m_state.enemyCrazy / 100.0, 0.0, 1.0);
			Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(12, 0, ColorF{ 0.85 });
			const double angle = Math::TwoPiF * ratio;
			Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 1.0, 0.4, 0.4 });
			const s3d::Texture& face = m_faces.select(m_state.enemyCrazy);
			const double s = 70.0;
			face.scaled(s / face.height()).drawAt(c);
		}

		// ===== 攻撃ボタン =====
		const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
		const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
		const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
		const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
		const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);
		const ColorF actionBg{ 1.0 };
		const bool disabledAll = m_state.defending;

		auto drawSlot = [&](const RoundRect& rr, int slot)
		{
			const bool isRefilling = m_deck.isSlotRefilling(slot);

			if (isRefilling)
			{
				rr.draw(ColorF{ 0.95 }).drawFrame(2);

				const double progress = m_deck.getSlotRefillProgress(slot);
				const Vec2 center = rr.center();
				const double radius = 30.0;

				Circle{ center, radius }.drawFrame(4, ColorF{ 0.7 });
				const double angle = Math::TwoPi * progress;
				Circle{ center, radius }.drawArc(-Math::HalfPi, angle, 4, 0, ColorF{ 0.2, 0.6, 1.0 });

				const double remainingSec = (1.0 - progress) * CardDeck::RefillCooldownSec;
				FontAsset(U"Bold")(U"{:.1f}"_fmt(remainingSec)).drawAt(18, center, ColorF{ 0.3 });
			}
			else
			{
				const bool hasCurrent = (slot < static_cast<int>(m_deck.current().size()));
				const bool hasLast    = (!hasCurrent && (slot < static_cast<int>(m_deck.lastDisplayed().size())));
				const bool hasAny     = hasCurrent || hasLast;

				const CardSpec* pCard = nullptr;
				if (hasCurrent) pCard = &m_deck.getVisualCard(slot);
				else if (hasLast) pCard = &m_deck.lastDisplayed()[slot];

				bool textureDrawn = false;
				if (pCard)
				{
					const Texture* pTex = nullptr;
					if      (pCard->attributeId == 1) pTex = &m_texCommandQuantity;
					else if (pCard->attributeId == 2) pTex = &m_texCommandQuality;
					else if (pCard->attributeId == 3) pTex = &m_texCommandCounter;

					if (pTex && *pTex)
					{
						pTex->resized(rr.rect.size).draw(rr.rect.pos);
						textureDrawn = true;
					}
				}

				if (!textureDrawn)
				{
					const ColorF base = disabledAll || !hasAny ? ColorF{ 0.95 } : actionBg;
					rr.draw(base).drawFrame(2);
				}

				String title;
				if (pCard)
					title = (pCard->name.isEmpty() ? U"攻撃{}"_fmt(slot + 1) : pCard->name);
				else
					title = U"攻撃{}"_fmt(slot + 1);

				const ColorF txt = disabledAll || !hasAny ? ColorF{ 0.5 } : ColorF{ 1 };

				const double padding = 8.0;
				const double maxWidth = rr.rect.w - padding * 2 - 10;
				const int32 normalFontSize = 17;
				const int32 smallFontSize  = 10;

				const RectF textRegion = FontAsset(U"Bold")(title).region(normalFontSize);
				if (textRegion.w > maxWidth)
					FontAsset(U"Bold")(title).drawAt(smallFontSize, rr.center(), txt);
				else
					FontAsset(U"Bold")(title).drawAt(normalFontSize, rr.center(), txt);
			}
		};

		drawSlot(attackBtn1, 0);
		drawSlot(attackBtn2, 1);
		drawSlot(attackBtn3, 2);
		drawSlot(attackBtn4, 3);

		escapeBtn.draw(ColorF{ 1.0, m_escapeTr.value() });
		BaseFrame().draw(escapeBtn.rect);
		FontAsset(U"Bold")(U"逃走").drawAt(24, escapeBtn.center(), ColorF{ 0.1 });

		// ===== コストパネル =====
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

		// ===== プレイヤーパネル（アイコン＋防御ボタン） =====
		const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
		const RoundRect panelRR{ playerPanel, BattleLayout::PlayerPanelR };
		panelRR.draw(ColorF{ 1.0, 0.95 });
		BaseFrame().draw(panelRR.rect);

		const RectF iconRect = BattleLayout::PlayerIconRect(playerPanel);
		iconRect.rounded(6).draw(ColorF{ 0.3, 0.7, 0.9 });
		if (m_texPlayerIcon)
		{
			m_texPlayerIcon.resized(iconRect.size).draw(iconRect.pos);
		}

		const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
		const bool isDefendingNow = m_state.defending;
		const s3d::Texture& guardTex = isDefendingNow ? m_texGuardOn : m_texGuardOff;
		if (guardTex)
		{
			guardTex.resized(defendBtn.rect.size).draw(defendBtn.rect.pos);
		}
		else
		{
			defendBtn.draw(ColorF{ 1.0 }).drawFrame(2);
			FontAsset(U"Bold")(U"防御").drawAt(24, defendBtn.center(), ColorF{ 0.1 });
		}

		// ===== イベントログ =====
		if (!m_eventLog.isEmpty())
		{
			const double panelMargin = 20.0;
			const double lineHeight  = 22.0;
			const size_t displayCount = m_eventLog.size();
			const double panelWidth  = sceneSize.x - (panelMargin * 2.0);
			const double panelHeight = 16.0 + (lineHeight * displayCount) + 16.0;
			const double panelX = panelMargin;
			const double panelY = sceneSize.y - 12.0 - panelHeight;

			const RoundRect logPanel{ RectF{ panelX, panelY, panelWidth, panelHeight }, 8.0 };
			logPanel.draw(ColorF{ 0.95, 0.95, 0.96, 0.9 }).drawFrame(2, 0, ColorF{ 0.2, 0.2, 0.3 });

			const double now = Scene::Time();
			Vec2 cursor{ panelX + 20.0, panelY + 18.0 };
			const size_t startIndex = (displayCount > MaxLogEntries) ? (displayCount - MaxLogEntries) : 0;

			for (size_t i = startIndex; i < displayCount; ++i)
			{
				const auto& entry = m_eventLog[i];
				const double age  = now - entry.timestamp;
				const double fade = Clamp(1.0 - (age / LogDisplayDuration), 0.0, 1.0);
				const double alpha = 0.35 + (0.65 * fade);
				const bool isLatest = (i + 1 == displayCount);
				const ColorF textColor = isLatest
					? ColorF{ 0.1, 0.1, 0.1, alpha }
					: ColorF{ 0.15, 0.15, 0.2, alpha };

				const double fontSize = isLatest ? 20.0 : 18.0;
				FontAsset(U"Bold")(entry.message).draw(fontSize, cursor, textColor);
				cursor.y += lineHeight;
			}
		}

		// ===== 詠唱ゲージ（プレイヤー） =====
		if (m_state.playerCasting)
		{
			const double progress = BattleLogic::getCastingProgress(m_state, true);
			const Vec2 gaugePos{ 100, 500 };
			const double gaugeWidth  = 200;
			const double gaugeHeight = 20;

			RectF{ gaugePos, gaugeWidth, gaugeHeight }.draw(ColorF{ 0.2, 0.2, 0.2 });
			RectF{ gaugePos, gaugeWidth * progress, gaugeHeight }.draw(ColorF{ 0.8, 0.6, 0.2 });
			FontAsset(U"Bold")(U"入力中: " + m_state.playerCastingCardName)
				.draw(24, Vec2{ gaugePos.x + 5, gaugePos.y - 25 }, ColorF{ 1.0 });
			const double remainingTime = m_state.playerCastDuration - m_state.playerCastTimer.sF();
			FontAsset(U"Bold")(U"{:.1f}秒"_fmt(remainingTime))
				.draw(20, Vec2{ gaugePos.x + gaugeWidth + 10, gaugePos.y + 2 }, ColorF{ 1.0 });
		}

		// ===== 敵の詠唱中表示 =====
		if (m_state.enemyCasting && m_texWriting)
		{
			const Vec2 imgPos = Vec2{ enemyPos.x + entitySize.x * 0.48, enemyPos.y + entitySize.y * 0.22};
			m_texWriting.draw(imgPos);
		}

		// ===== カード飛翔演出 =====
		auto drawProjectile = [&](const CardProjectile& proj)
		{
			if (!proj.active) return;

			const double elapsed = proj.timer.sF();

			if (proj.isBlinking)
			{
				const double blinkElapsed  = elapsed - CardProjectile::FlightDuration;
				const double blinkProgress = blinkElapsed / CardProjectile::BlinkDuration;
				const bool isVisible = (static_cast<int>(blinkElapsed * 10.0) % 2) == 0;

				if (isVisible)
				{
					const Vec2 currentPos = proj.targetPos;
					const double cardSize = 80.0 * 0.7;
					const RoundRect cardRect{ Arg::center = currentPos, cardSize, cardSize * 0.6, 8.0 };
					const double flashIntensity = 1.0 - blinkProgress * 0.3;
					cardRect.draw(ColorF{ 1.0, 1.0, 0.9, flashIntensity }).drawFrame(2, ColorF{ 1.0, 0.8, 0.2, flashIntensity });
					FontAsset(U"Bold")(proj.cardName).drawAt(14.0, currentPos, ColorF{ 0.1, 0.1, 0.2, flashIntensity });
				}
			}
			else
			{
				const double t = Clamp(elapsed / CardProjectile::FlightDuration, 0.0, 1.0);
				const double eased = EaseInOutQuad(t);
				const Vec2 currentPos = proj.startPos.lerp(proj.targetPos, eased);

				const double cardSize = 80.0 * (1.0 - 0.3 * t);
				const RoundRect cardRect{ Arg::center = currentPos, cardSize, cardSize * 0.6, 8.0 };
				cardRect.draw(ColorF{ 1.0, 1.0, 0.9, 0.95 - 0.3 * t }).drawFrame(2, ColorF{ 0.2, 0.2, 0.3 });
				FontAsset(U"Bold")(proj.cardName).drawAt(16.0 * (1.0 - 0.2 * t), currentPos, ColorF{ 0.1, 0.1, 0.2, 0.9 - 0.4 * t });

				for (int i = 1; i <= 3; ++i)
				{
					const double trailT = Clamp(t - i * 0.08, 0.0, 1.0);
					if (trailT <= 0.0) continue;
					const double trailEased = EaseInOutQuad(trailT);
					const Vec2 trailPos = proj.startPos.lerp(proj.targetPos, trailEased);
					const double alpha = 0.3 * (1.0 - t) * (1.0 - i * 0.25);
					const double trailSize = cardSize * (1.0 - i * 0.15);
					RoundRect{ Arg::center = trailPos, trailSize, trailSize * 0.6, 8.0 }
						.draw(ColorF{ 1.0, 1.0, 0.9, alpha });
				}
			}
		};

		drawProjectile(m_playerProjectile);
		drawProjectile(m_enemyProjectile);
	}

	// ===== ポーズエフェクト =====
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

	ScreenFrame().draw(s3d::RectF{ 0, 0, static_cast<double>(Scene::Width()), static_cast<double>(Scene::Height()) });
}
