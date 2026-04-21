#include "../scenes/Game_Impl.hpp"
#include "BattleLogic.hpp"
#include "BattleUtils.hpp"

// =============================================================================
//  フレーム更新・入力収集・各種ユーティリティ
// =============================================================================

void Game::update()
{
	if (KeyEscape.down())
	{
		m_paused = (not m_paused);
	}

	if (m_paused)
	{
		updatePausedUI();
		return;
	}

	if (m_isOnlineMode && m_multiplayer)
	{
		m_multiplayer->update();
	}

	const double dt = Scene::DeltaTime();

	if (!BattleLogic::isRegenBlocked(m_state))
	{
		BattleLogic::regenCost(m_state, dt);
	}

	if (m_state.defending && (m_state.defendTimer.sF() >= BattleState::DefendDurationSec))
	{
		m_state.defending = false;
	}

	updateRemoteDefendState();
	updateRemoteCost(dt);
	updateLogs();
	updateProjectiles();

	// 攻撃アニメーションの終了判定
	if (m_playerAttackAnimActive && m_playerAttackAnimTimer.sF() >= AttackAnimTotalDuration)
	{
		m_playerAttackAnimActive = false;
	}
	if (m_enemyAttackAnimActive && m_enemyAttackAnimTimer.sF() >= AttackAnimTotalDuration)
	{
		m_enemyAttackAnimActive = false;
	}

	// クレイジーズーム演出の終了判定
	if (m_crazyZoom.active && m_crazyZoom.timer.sF() >= CrazyZoomEffect::Duration)
	{
		m_crazyZoom.active = false;
	}

	// 弱点シェイク演出の終了判定
	if (m_weaknessShake.active && m_weaknessShake.timer.sF() >= WeaknessShakeEffect::Duration)
	{
		m_weaknessShake.active = false;
	}

	m_deck.updateRefills();

	// ===== クレイジーモード終了チェック =====
	// PvP クライアントはホストが権威のため自律終了しない（applyStateSync 経由で終了）
	const double currentTime = Scene::Time();
	if (!m_isOnlineMode || m_isHost)
	{
		if (BattleLogic::shouldExitCrazyMode(m_state, true, currentTime))
		{
			BattleLogic::endCrazyMode(m_state, true);
			m_deck.exitCrazyMode();
			pushLog(U"【CRAZY MODE 終了】");
			if (m_isOnlineMode && m_isHost) sendStateSync();
		}
		if (BattleLogic::shouldExitCrazyMode(m_state, false, currentTime))
		{
			BattleLogic::endCrazyMode(m_state, false);
			if (m_isOnlineMode && m_isHost) sendStateSync();
		}
	}

	BattleInput input = collectBattleInput();

	if (m_loop)
	{
		m_loop->update(input);
	}

	finishBattleIfNeeded();
}

Game::BattleInput Game::collectBattleInput()
{
	BattleInput input;
	const Size sceneSize = Scene::Size();

	const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
	const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
	const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
	const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
	const RoundRect escapeBtn  = BattleLayout::EscapeButton(sceneSize);
	const RectF playerPanel    = BattleLayout::PlayerPanelRect(sceneSize);
	const RoundRect defendBtn  = BattleLayout::DefendButtonRect(playerPanel);

	const bool canClickAttack1 = attackBtn1.mouseOver() && !m_deck.isSlotRefilling(0);
	const bool canClickAttack2 = attackBtn2.mouseOver() && !m_deck.isSlotRefilling(1);
	const bool canClickAttack3 = attackBtn3.mouseOver() && !m_deck.isSlotRefilling(2);
	const bool canClickAttack4 = attackBtn4.mouseOver() && !m_deck.isSlotRefilling(3);

	if (canClickAttack1 || canClickAttack2 || canClickAttack3 || canClickAttack4 || escapeBtn.mouseOver() || defendBtn.mouseOver())
	{
		Cursor::RequestStyle(CursorStyle::Hand);
	}

	input.attack[0] = attackBtn1.leftClicked() && !m_deck.isSlotRefilling(0);
	input.attack[1] = attackBtn2.leftClicked() && !m_deck.isSlotRefilling(1);
	input.attack[2] = attackBtn3.leftClicked() && !m_deck.isSlotRefilling(2);
	input.attack[3] = attackBtn4.leftClicked() && !m_deck.isSlotRefilling(3);
	input.escape = escapeBtn.leftClicked();
	input.defend = defendBtn.leftClicked();

	return input;
}

void Game::updateLogs()
{
	const double now = Scene::Time();

	for (int32 i = static_cast<int32>(m_eventLog.size()) - 1; i >= 0; --i)
	{
		if ((now - m_eventLog[i].timestamp) > LogDisplayDuration)
		{
			m_eventLog.erase(m_eventLog.begin() + i);
		}
	}
}

void Game::pushLog(const String& message)
{
	m_eventLog.emplace_back(LogEntry{ message, Scene::Time() });

	while (m_eventLog.size() > MaxLogEntries)
	{
		m_eventLog.erase(m_eventLog.begin());
	}
}

void Game::updateRemoteCost(double deltaTime)
{
	if (!m_isHost)
	{
		return;
	}

	if (!m_remoteDefending)
	{
		m_remoteCostValue = Min(100.0, m_remoteCostValue + (BattleState::CostRegenPerSec * deltaTime));
	}
}

void Game::updateRemoteDefendState()
{
	if (!m_isHost)
	{
		return;
	}

	if (m_remoteDefending && (Scene::Time() >= m_remoteDefendEndTime))
	{
		m_remoteDefending = false;
	}
}

double Game::remoteAvailableCost() const
{
	return m_isHost ? m_remoteCostValue : 0.0;
}

void Game::consumeRemoteCost(double amount)
{
	if (!m_isHost)
	{
		return;
	}

	m_remoteCostValue = Max(0.0, m_remoteCostValue - amount);
}

void Game::startRemoteDefend()
{
	if (!m_isHost)
	{
		return;
	}

	m_remoteDefending = true;
	m_remoteDefendEndTime = Scene::Time() + BattleState::DefendDurationSec;
}

void Game::performPvEEnemyCounter()
{
	const int32 baseDamage = s3d::Random(8, 16);
	const int32 finalDamage = m_state.defending ? 0 : baseDamage;
	handleEnemyAttack(-1, finalDamage, net::BattleEventType::ClientAttackDamage, false);
}

void Game::finishBattleIfNeeded()
{
	if ((m_state.playerHP <= 0) || (m_state.enemyHP <= 0))
	{
		concludeBattle(net::BattleEndReason::HPZero, (m_state.playerHP > 0));
	}
}

void Game::concludeBattle(net::BattleEndReason reason, bool hostWon)
{
	if (m_battleEnded) return;
	m_battleEnded = true;
	if (m_isOnlineMode && m_multiplayer && m_multiplayer->isConnected() && m_isHost)
	{
		net::BattleEndMessage msg{};
		msg.hostFinalHP = m_state.playerHP;
		msg.clientFinalHP = m_state.enemyHP;
		msg.hostWon = hostWon ? 1 : 0;
		msg.reason = reason;
		m_multiplayer->send(msg);
	}
	getData().lastResult = hostWon == m_isHost;
	if (getData().multiplayer)
	{
		getData().multiplayer.reset();
	}
	getData().lastMode = m_isOnlineMode ? GameData::GameMode::PvP : GameData::GameMode::PvE;
	m_isOnlineMode = false;
	m_isHost = false;

	changeScene(State::Result);
}

void Game::replaceUsedCardIfNeeded()
{
	if (m_deck.hasPendingReplacement())
	{
		m_deck.replaceUsedCard();
	}
}

void Game::startPlayerProjectile(int32 slotIndex, const String& cardName, bool isBlocked)
{
	const Size sceneSize = Scene::Size();

	const RoundRect attackBtn = BattleLayout::AttackOptionButton(sceneSize, slotIndex);
	m_playerProjectile.startPos = attackBtn.center();

	const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;
	m_playerProjectile.targetPos = enemyPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };

	m_playerProjectile.slotIndex = slotIndex;
	m_playerProjectile.cardName = cardName;
	m_playerProjectile.isBlocked = isBlocked;
	m_playerProjectile.timer.restart();
	m_playerProjectile.active = true;
}

void Game::startEnemyProjectile(int32 slotIndex, const String& cardName, bool isBlocked)
{
	const Size sceneSize = Scene::Size();

	const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;
	m_enemyProjectile.startPos = enemyPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };

	const Vec2 playerPos = BattleLayout::PlayerPos(sceneSize);
	m_enemyProjectile.targetPos = playerPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };

	m_enemyProjectile.slotIndex = slotIndex;
	m_enemyProjectile.cardName = cardName;
	m_enemyProjectile.isBlocked = isBlocked;
	m_enemyProjectile.timer.restart();
	m_enemyProjectile.active = true;
}

void Game::updateProjectiles()
{
	// 防御エフェクトは着弾の FlightDuration/5 秒前に先出し
	static constexpr double GuardEarlyTrigger = CardProjectile::FlightDuration / 5.0;

	// プレイヤーの飛翔演出を更新
	if (m_playerProjectile.active)
	{
		const double elapsed = m_playerProjectile.timer.sF();

		// 防御エフェクト先出し
		if (m_playerProjectile.isBlocked && !m_playerProjectile.guardEffectTriggered
			&& elapsed >= CardProjectile::FlightDuration - GuardEarlyTrigger)
		{
			m_playerProjectile.guardEffectTriggered = true;
			triggerGuardEffect(false);
		}

		if (!m_playerProjectile.isBlinking && elapsed >= CardProjectile::FlightDuration)
		{
			m_playerProjectile.isBlinking = true;
			// 到達タイミング：ダメージ・演出を適用
			applyPendingImpact(m_playerProjectile.pendingImpact);
			// 防御成功時は着弾と同時に消す
			if (m_playerProjectile.isBlocked)
			{
				m_playerProjectile.active = false;
				m_playerProjectile.isBlinking = false;
				m_playerProjectile.isBlocked = false;
				m_playerProjectile.guardEffectTriggered = false;
			}
		}

		if (m_playerProjectile.active && elapsed >= CardProjectile::TotalDuration)
		{
			m_playerProjectile.active = false;
			m_playerProjectile.isBlinking = false;
			m_playerProjectile.isBlocked = false;
			m_playerProjectile.guardEffectTriggered = false;
		}
	}

	// 敵の飛翔演出を更新
	if (m_enemyProjectile.active)
	{
		const double elapsed = m_enemyProjectile.timer.sF();

		// 防御エフェクト先出し
		if (m_enemyProjectile.isBlocked && !m_enemyProjectile.guardEffectTriggered
			&& elapsed >= CardProjectile::FlightDuration - GuardEarlyTrigger)
		{
			m_enemyProjectile.guardEffectTriggered = true;
			triggerGuardEffect(true);
		}

		if (!m_enemyProjectile.isBlinking && elapsed >= CardProjectile::FlightDuration)
		{
			m_enemyProjectile.isBlinking = true;
			// 到達タイミング：ダメージ・演出を適用
			applyPendingImpact(m_enemyProjectile.pendingImpact);
			// 防御成功時は着弾と同時に消す
			if (m_enemyProjectile.isBlocked)
			{
				m_enemyProjectile.active = false;
				m_enemyProjectile.isBlinking = false;
				m_enemyProjectile.isBlocked = false;
				m_enemyProjectile.guardEffectTriggered = false;
			}
		}

		if (m_enemyProjectile.active && elapsed >= CardProjectile::TotalDuration)
		{
			m_enemyProjectile.active = false;
			m_enemyProjectile.isBlinking = false;
			m_enemyProjectile.isBlocked = false;
			m_enemyProjectile.guardEffectTriggered = false;
		}
	}
}

void Game::applyPendingImpact(PendingImpact& impact)
{
	if (!impact.valid) return;
	impact.valid = false;

	// HP ダメージ適用
	if (impact.hpChange > 0)
	{
		if (impact.targetIsPlayer)
			m_state.playerHP = Max(0, m_state.playerHP - impact.hpChange);
		else
			m_state.enemyHP = Max(0, m_state.enemyHP - impact.hpChange);
	}

	// クレイジーゲージ増加
	if (impact.crazyGain > 0)
		BattleLogic::addCrazy(m_state, impact.crazyTargetIsEnemy, impact.crazyGain);

	// ログ出力・フラッシュエフェクト（emitLocalEvent 内で処理）
	emitLocalEvent(impact.eventType, impact.primaryValue, impact.secondaryValue, impact.flags);

	// ステート同期（PvP ホスト）
	if (impact.shouldSendStateSync)
		sendStateSync();

	// バトル終了判定
	if (impact.shouldCheckBattleEnd)
		finishBattleIfNeeded();
}

void Game::triggerGuardEffect(bool isPlayer)
{
	const Size sceneSize = Scene::Size();
	m_guardEffect.active = true;
	m_guardEffect.timer.restart();
	m_guardEffect.pos = isPlayer
		? BattleLayout::PlayerPos(sceneSize) + Vec2{ BattleLayout::EntitySize } * 0.5
		: BattleLayout::EnemyPos(sceneSize)  + Vec2{ BattleLayout::EntitySize } * 0.5;
}

void Game::triggerCrazyZoom(bool isPlayer)
{
	m_crazyZoom.isPlayer = isPlayer;
	m_crazyZoom.active   = true;
	m_crazyZoom.timer.restart();
}

void Game::triggerWeaknessShake(bool isPlayer)
{
	m_weaknessShake.isPlayer = isPlayer;
	m_weaknessShake.active   = true;
	m_weaknessShake.timer.restart();
}

void Game::updatePausedUI()
{
	m_pauseMenu.setActions({
		PauseMenu::Action::Resume,
		PauseMenu::Action::Lobby,
		PauseMenu::Action::Title,
		PauseMenu::Action::Exit,
	});

	const PauseMenu::Action action = m_pauseMenu.update();
	getData().pauseReturnState = State::Lobby;
	switch (action)
	{
	case PauseMenu::Action::Resume:
		m_paused = false;
		break;
	case PauseMenu::Action::Lobby:
		concludeBattle(net::BattleEndReason::Escape, !m_isHost);
		changeScene(State::Lobby);
		break;
	case PauseMenu::Action::Title:
		concludeBattle(net::BattleEndReason::Escape, !m_isHost);
		changeScene(State::Title);
		break;
	case PauseMenu::Action::Exit:
		concludeBattle(net::BattleEndReason::Escape, !m_isHost);
		System::Exit();
		break;
	default:
		break;
	}
}
