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

	m_deck.updateRefills();

	// ===== クレイジーモード終了チェック =====
	const double currentTime = Scene::Time();
	if (BattleLogic::shouldExitCrazyMode(m_state, true, currentTime))
	{
		BattleLogic::endCrazyMode(m_state, true);
		m_deck.exitCrazyMode();
		pushLog(U"【CRAZY MODE 終了】");
	}
	if (BattleLogic::shouldExitCrazyMode(m_state, false, currentTime))
	{
		BattleLogic::endCrazyMode(m_state, false);
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

void Game::startPlayerProjectile(int32 slotIndex, const String& cardName)
{
	const Size sceneSize = Scene::Size();

	const RoundRect attackBtn = BattleLayout::AttackOptionButton(sceneSize, slotIndex);
	m_playerProjectile.startPos = attackBtn.center();

	const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;
	m_playerProjectile.targetPos = enemyPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };

	m_playerProjectile.slotIndex = slotIndex;
	m_playerProjectile.cardName = cardName;
	m_playerProjectile.timer.restart();
	m_playerProjectile.active = true;
}

void Game::startEnemyProjectile(int32 slotIndex, const String& cardName)
{
	const Size sceneSize = Scene::Size();

	const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;
	m_enemyProjectile.startPos = enemyPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };

	const Vec2 playerPos = BattleLayout::PlayerPos(sceneSize);
	m_enemyProjectile.targetPos = playerPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };

	m_enemyProjectile.slotIndex = slotIndex;
	m_enemyProjectile.cardName = cardName;
	m_enemyProjectile.timer.restart();
	m_enemyProjectile.active = true;
}

void Game::updateProjectiles()
{
	// プレイヤーの飛翔演出を更新
	if (m_playerProjectile.active)
	{
		const double elapsed = m_playerProjectile.timer.sF();

		if (!m_playerProjectile.isBlinking && elapsed >= CardProjectile::FlightDuration)
		{
			m_playerProjectile.isBlinking = true;
		}

		if (elapsed >= CardProjectile::TotalDuration)
		{
			m_playerProjectile.active = false;
			m_playerProjectile.isBlinking = false;
		}
	}

	// 敵の飛翔演出を更新
	if (m_enemyProjectile.active)
	{
		const double elapsed = m_enemyProjectile.timer.sF();

		if (!m_enemyProjectile.isBlinking && elapsed >= CardProjectile::FlightDuration)
		{
			m_enemyProjectile.isBlinking = true;
		}

		if (elapsed >= CardProjectile::TotalDuration)
		{
			m_enemyProjectile.active = false;
			m_enemyProjectile.isBlinking = false;
		}
	}
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
