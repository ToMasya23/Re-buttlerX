#include "Game.hpp"
#include "../game/BattleLogic.hpp"
#include "../game/BattleUtils.hpp"
#include "../tools/NineSlice.hpp"

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

	inline ActionType actionTypeFromSlot(int slot)
	{
		switch (slot)
		{
		case 0: return ActionType::Attack1;
		case 1: return ActionType::Attack2;
		case 2: return ActionType::Attack3;
		case 3: return ActionType::Attack4;
		default: return ActionType::Attack1;
		}
	}
}

class Game::PvELoop : public Game::BattleLoop
{
public:
	using Game::BattleLoop::BattleLoop;

	void update(const BattleInput& input) override
	{
		Game& g = m_game;

		// ===== クレイジーモード発動チェック =====
		if (BattleLogic::shouldEnterCrazyMode(g.m_state, true))
		{
			BattleLogic::startCrazyMode(g.m_state, true, Scene::Time());
			g.m_deck.enterCrazyMode();
			g.pushLog(U"【CRAZY MODE 発動！】");
		}

		const auto& cards = g.m_deck.current();

		auto tryAttack = [&](int slotIndex) -> bool
		{
			if (slotIndex < 0 || slotIndex >= static_cast<int>(cards.size()))
			{
				return false;
			}

			const CardSpec& visualCard = g.m_deck.getVisualCard(slotIndex);

			if (!BattleLogic::canAttack(g.m_state))
			{
				g.handleActionRejected(1, net::BattleEventType::HostActionRejected, false);
				return true;
			}

			if (!BattleLogic::trySpendCost(g.m_state, visualCard.cost))
			{
				g.handleActionRejected(0, net::BattleEventType::HostActionRejected, false);
				return true;
			}

			const int32 damage = BattleUtils::slotDamage(slotIndex, g.m_deck);
			g.handlePlayerAttack(slotIndex, damage, net::BattleEventType::HostAttackDamage, false);
			g.performPvEEnemyCounter();
			return true;
		};

		for (int i = 0; i < 4; ++i)
		{
			if (input.attack[i] && tryAttack(i))
			{
				return;
			}
		}

		if (input.escape)
		{
			g.handleEscape(net::BattleEventType::HostEscape, false);
			return;
		}

		if (input.defend)
		{
			if (!BattleLogic::canDefend(g.m_state) || !BattleLogic::trySpendCost(g.m_state, 20))
			{
				g.handleActionRejected(0, net::BattleEventType::HostActionRejected, false);
				return;
			}

			g.handleDefend(net::BattleEventType::HostDefend, false);
		}
	}

};

class Game::PvPHostLoop : public Game::BattleLoop
{
public:
	using Game::BattleLoop::BattleLoop;

	void onEnter() override
	{
		if (m_game.m_multiplayer && m_game.m_multiplayer->isConnected())
		{
			m_game.sendStateSync();
			m_lastSyncSent = Scene::Time();
		}
	}

	void update(const BattleInput& input) override
	{
		processClientMessages();

		Game& g = m_game;

		// ===== クレイジーモード発動チェック =====
		if (BattleLogic::shouldEnterCrazyMode(g.m_state, true))
		{
			BattleLogic::startCrazyMode(g.m_state, true, Scene::Time());
			g.m_deck.enterCrazyMode();
			g.pushLog(U"【CRAZY MODE 発動！】");
			g.sendStateSync();
		}

		const auto& cards = g.m_deck.current();

		// ===== 詠唱完了チェック（ホスト側） =====
		if (BattleLogic::isCastingComplete(g.m_state, true))
		{
			int32 slot = g.m_state.playerCastingSlot;
			const int32 damage = BattleUtils::slotDamage(slot, g.m_deck);
			g.handlePlayerAttack(slot, damage, net::BattleEventType::HostAttackDamage, true);
			BattleLogic::cancelCasting(g.m_state, true);
			g.sendStateSync();
		}

		// ===== 詠唱完了チェック（クライアント側） =====
		if (BattleLogic::isCastingComplete(g.m_state, false))
		{
			int32 slot = g.m_state.enemyCastingSlot;
			const int32 damage = BattleUtils::slotDamage(slot, g.m_deck);
			g.handleEnemyAttack(slot, damage, net::BattleEventType::ClientAttackDamage, true);
			BattleLogic::cancelCasting(g.m_state, false);
			g.sendStateSync();
		}

		auto tryAttack = [&](int slotIndex) -> bool
		{
			if (slotIndex < 0 || slotIndex >= static_cast<int>(cards.size()))
			{
				return false;
			}

			// 詠唱中は新しい行動不可
			if (g.m_state.playerCasting)
			{
				g.handleActionRejected(2, net::BattleEventType::HostActionRejected, true);
				return true;
			}

			const CardSpec& visualCard = g.m_deck.getVisualCard(slotIndex);
			if (!BattleLogic::canAttack(g.m_state))
			{
				g.handleActionRejected(1, net::BattleEventType::HostActionRejected, true);
				return true;
			}

			if (!BattleLogic::trySpendCost(g.m_state, visualCard.cost))
			{
				g.handleActionRejected(0, net::BattleEventType::HostActionRejected, true);
				return true;
			}

			// 詠唱開始（即座にダメージは与えない）
			double castTime = BattleLogic::calculateCastTime(visualCard.name);
			BattleLogic::startCasting(g.m_state, true, slotIndex, visualCard.name, castTime);
			g.pushLog(U"「" + visualCard.name + U"」を詠唱中...");
			g.sendStateSync();
			return true;
		};

		for (int i = 0; i < 4; ++i)
		{
			if (input.attack[i] && tryAttack(i))
			{
				return;
			}
		}

		if (input.escape)
		{
			g.handleEscape(net::BattleEventType::HostEscape, true);
			return;
		}

		if (input.defend)
		{
			if (!BattleLogic::canDefend(g.m_state) || !BattleLogic::trySpendCost(g.m_state, 20))
			{
				g.handleActionRejected(0, net::BattleEventType::HostActionRejected, true);
				return;
			}

			// 詠唱キャンセル
			if (g.m_state.playerCasting)
			{
				BattleLogic::cancelCasting(g.m_state, true);
				g.pushLog(U"詠唱をキャンセルして防御！");
			}

			g.handleDefend(net::BattleEventType::HostDefend, true);
			return;
		}

		const double now = Scene::Time();
		if (g.m_multiplayer && g.m_multiplayer->isConnected() && (now - m_lastSyncSent) > 0.2)
		{
			g.sendStateSync();
			m_lastSyncSent = now;
		}
	}

private:
	void processClientMessages()
	{
		if (!m_game.m_multiplayer || m_game.m_multiplayer->getConnectionState() != MultiplayerManager::ConnectionState::Connected)
		{
			return;
		}

		auto& net = *m_game.m_multiplayer;

		while (true)
		{
			net::PacketType type = net.peekPacketType();
			if (type == net::PacketType::ActionRequest)
			{
				auto msg = net.receive<net::ActionRequestMessage>();
				if (!msg)
				{
					break;
				}
				handleClientRequest(*msg);
			}
			else if (type == net::PacketType::BattleEvent || type == net::PacketType::StateSnapshot || type == net::PacketType::BattleEnd)
			{
				net.discardFrontPacket();
			}
			else
			{
				break;
			}
		}
	}

	void handleClientRequest(const net::ActionRequestMessage& msg)
	{
		Game& g = m_game;

		if (msg.requestId <= m_lastClientRequestId)
		{
			return;
		}

		m_lastClientRequestId = msg.requestId;

		const auto& cards = g.m_deck.current();

		switch (msg.action)
		{
		case ActionType::Attack1:
		case ActionType::Attack2:
		case ActionType::Attack3:
		case ActionType::Attack4:
		{
			const int slotIndex = Clamp(static_cast<int>(msg.slotIndex), 0, 3);

			// クライアントが詠唱中なら拒否
			if (g.m_state.enemyCasting)
			{
				g.handleActionRejected(2, net::BattleEventType::ClientActionRejected, true);
				return;
			}

			const CardSpec& visualCard = g.m_deck.getVisualCard(slotIndex);
			const double cost = (slotIndex >= 0 && slotIndex < static_cast<int>(cards.size())) ? visualCard.cost : 20.0;
			if (g.remoteAvailableCost() < cost)
			{
				g.handleActionRejected(0, net::BattleEventType::ClientActionRejected, true);
				return;
			}

			g.consumeRemoteCost(cost);

			// 詠唱開始（クライアント側）
			if (slotIndex >= 0 && slotIndex < static_cast<int>(cards.size()))
			{
				double castTime = BattleLogic::calculateCastTime(visualCard.name);
				BattleLogic::startCasting(g.m_state, false, slotIndex, visualCard.name, castTime);
				g.sendStateSync();
			}
			break;
		}
		case ActionType::Defend:
		{
			const double defendCost = 20.0;
			if (g.remoteAvailableCost() < defendCost)
			{
				g.handleActionRejected(0, net::BattleEventType::ClientActionRejected, true);
				return;
			}

			g.consumeRemoteCost(defendCost);
			g.startRemoteDefend();
			g.handleDefend(net::BattleEventType::ClientDefend, true);
			break;
		}
		case ActionType::Skill:
		{
			g.handleEscape(net::BattleEventType::ClientEscape, true);
			break;
		}
		default:
			g.handleActionRejected(3, net::BattleEventType::ClientActionRejected, true);
			break;
		}
	}

	uint32 m_lastClientRequestId = 0;
	double m_lastSyncSent = 0.0;
};

class Game::PvPClientLoop : public Game::BattleLoop
{
public:
	using Game::BattleLoop::BattleLoop;

	void update(const BattleInput& input) override
	{
		Game& g = m_game;

		processIncomingPackets();

		// ===== クレイジーモード発動チェック（クライアント側） =====
		if (BattleLogic::shouldEnterCrazyMode(g.m_state, true))
		{
			BattleLogic::startCrazyMode(g.m_state, true, Scene::Time());
			g.m_deck.enterCrazyMode();
			g.pushLog(U"【CRAZY MODE 発動！】");
		}

		const auto& cards = g.m_deck.current();

		auto trySendAttack = [&](int slotIndex) -> bool
		{
			if (slotIndex < 0 || slotIndex >= static_cast<int>(cards.size()))
			{
				return false;
			}

			// 詠唱中は新しい行動不可
			if (g.m_state.playerCasting)
			{
				g.handleActionRejected(2, net::BattleEventType::ClientActionRejected, false);
				return true;
			}

			const CardSpec& visualCard = g.m_deck.getVisualCard(slotIndex);
			if (!BattleLogic::canAttack(g.m_state))
			{
				g.handleActionRejected(1, net::BattleEventType::ClientActionRejected, false);
				return true;
			}

			if (g.m_state.cost() < visualCard.cost)
			{
				g.handleActionRejected(0, net::BattleEventType::ClientActionRejected, false);
				return true;
			}

			sendRequest(actionTypeFromSlot(slotIndex), slotIndex);
			g.pushLog(U"「" + visualCard.name + U"」を詠唱要求...");
			return true;
		};

		for (int i = 0; i < 4; ++i)
		{
			if (input.attack[i] && trySendAttack(i))
			{
				return;
			}
		}

		if (input.escape)
		{
			sendRequest(ActionType::Skill, 0);
			return;
		}

		if (input.defend)
		{
			if (!BattleLogic::canDefend(g.m_state) || g.m_state.cost() < 20)
			{
				g.handleActionRejected(0, net::BattleEventType::ClientActionRejected, false);
				return;
			}

			// 詠唱キャンセル
			if (g.m_state.playerCasting)
			{
				BattleLogic::cancelCasting(g.m_state, true);
				g.pushLog(U"詠唱をキャンセルして防御！");
			}

			sendRequest(ActionType::Defend, 0);
		}
	}

private:
	void sendRequest(ActionType action, int slotIndex)
	{
		if (!m_game.m_multiplayer || m_game.m_multiplayer->getConnectionState() != MultiplayerManager::ConnectionState::Connected)
		{
			return;
		}

		net::ActionRequestMessage msg{};
		msg.requestId = m_nextRequestId++;
		msg.action = action;
		msg.slotIndex = static_cast<uint8>(slotIndex);
		m_game.m_multiplayer->send(msg);
	}

	void processIncomingPackets()
	{
		if (!m_game.m_multiplayer || m_game.m_multiplayer->getConnectionState() != MultiplayerManager::ConnectionState::Connected)
		{
			return;
		}

		auto& net = *m_game.m_multiplayer;

		while (true)
		{
			net::PacketType type = net.peekPacketType();
			if (type == net::PacketType::StateSnapshot)
			{
				auto msg = net.receive<net::StateSnapshotMessage>();
				if (!msg)
				{
					break;
				}
				m_game.applyStateSync(*msg);
			}
			else if (type == net::PacketType::BattleEvent)
			{
				auto msg = net.receive<net::BattleEventMessage>();
				if (!msg)
				{
					break;
				}
				m_game.emitLocalEvent(msg->eventType, msg->primaryValue, msg->secondaryValue, msg->flags);
				
				// クライアント側で自分の攻撃イベントを受信した場合、カードを更新＆演出開始
				if (msg->eventType == net::BattleEventType::ClientAttackDamage || 
				    msg->eventType == net::BattleEventType::ClientAttackBlocked)
				{
					int32 slotIndex = msg->secondaryValue;
					if (slotIndex >= 0 && slotIndex < 4)
					{
						m_game.m_deck.onUse(slotIndex);
						m_game.replaceUsedCardIfNeeded();
						
						// カード飛翔演出を開始
						if (slotIndex < static_cast<int>(m_game.m_deck.current().size()))
						{
							const CardSpec& visualCard = m_game.m_deck.getVisualCard(slotIndex);
							String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
							m_game.startPlayerProjectile(slotIndex, cardName);
						}
					}
				}
				// クライアント側でホストの攻撃イベントを受信した場合、敵側の演出開始
				else if (msg->eventType == net::BattleEventType::HostAttackDamage || 
				         msg->eventType == net::BattleEventType::HostAttackBlocked)
				{
					int32 slotIndex = msg->secondaryValue;
					// 敵からの攻撃演出を開始
					if (slotIndex >= 0 && slotIndex < static_cast<int>(m_game.m_deck.current().size()))
					{
						const CardSpec& visualCard = m_game.m_deck.getVisualCard(slotIndex);
						String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
						m_game.startEnemyProjectile(slotIndex, cardName);
					}
				}
				// クライアント側で自分の防御イベントを受信した場合、防御状態を設定
				else if (msg->eventType == net::BattleEventType::ClientDefend)
				{
					m_game.m_state.defending = true;
					m_game.m_state.defendTimer.restart();
				}
			}
			else if (type == net::PacketType::BattleEnd)
			{
				auto msg = net.receive<net::BattleEndMessage>();
				if (msg)
				{
					const bool hostWon = (msg->hostWon != 0);
					m_game.concludeBattle(msg->reason, hostWon);
				}
			}
			else
			{
				break;
			}
		}
	}

	uint32 m_nextRequestId = 1;
};
Game::Game(const InitData& init)
	: IScene{ init }
{
	m_faces.load();
	m_deck.loadAll();
	if (m_deck.hasCards())
	{
		m_deck.refillRandom(4);
	}

	m_texPlayer = s3d::Texture{ U"assets/ui/characters/player.png", s3d::TextureDesc::Unmipped };
	m_texEnemy = s3d::Texture{ U"assets/ui/characters/enemy.png", s3d::TextureDesc::Unmipped };

	if (getData().multiplayer)
	{
		m_multiplayer = getData().multiplayer;
		m_isOnlineMode = true;
		m_isHost = getData().isHost;
	}

	m_remoteCostValue = 100.0;
	m_remoteDefending = false;
	m_remoteDefendEndTime = 0.0;

	setupBattleLoop();
	if (m_loop)
	{
		m_loop->onEnter();
	}
}

void Game::setupBattleLoop()
{
	if (m_isOnlineMode)
	{
		if (m_isHost)
		{
			m_loop = std::make_unique<PvPHostLoop>(*this);
		}
		else
		{
			m_loop = std::make_unique<PvPClientLoop>(*this);
		}
	}
	else
	{
		m_loop = std::make_unique<PvELoop>(*this);
	}
}

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
	const RoundRect escapeBtn = BattleLayout::EscapeButton(sceneSize);
	const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
	const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);

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
	Console << U"LastResult: " << getData().lastResult;
	// if (m_multiplayer)
	// {
	// 	m_multiplayer->disconnect();
	// }
	// m_multiplayer.reset();
	if (getData().multiplayer)
	{
		getData().multiplayer.reset();
	}
	getData().lastMode = m_isOnlineMode ? GameData::GameMode::PvP : GameData::GameMode::PvE;
	m_isOnlineMode = false;
	m_isHost = false;

	changeScene(State::Result);
}

void Game::sendStateSync()
{
	if (!(m_isOnlineMode && m_isHost && m_multiplayer && m_multiplayer->isConnected()))
	{
		return;
	}

	net::StateSnapshotMessage msg{};
	msg.hostHP = m_state.playerHP;
	msg.clientHP = m_state.enemyHP;
	msg.hostCost = static_cast<float>(m_state.costValue);
	msg.hostCrazy = m_state.playerCrazy;
	msg.clientCrazy = m_state.enemyCrazy;
	msg.clientCost = static_cast<float>(m_remoteCostValue);
	msg.isHostTurn = 0;
	msg.turnNumber = 0;

	// ホスト側の状態エンコード（0=通常, 1=防御, 2=詠唱）
	if (m_state.playerCasting)
	{
		msg.hostDefending = 2;
		msg.hostDefendTime = static_cast<float>(Max(0.0, m_state.playerCastDuration - m_state.playerCastTimer.sF()));
	}
	else if (m_state.defending)
	{
		msg.hostDefending = 1;
		msg.hostDefendTime = static_cast<float>(m_state.defendTimer.sF());
	}
	else
	{
		msg.hostDefending = 0;
		msg.hostDefendTime = 0.0f;
	}

	// クライアント側の状態エンコード
	if (m_state.enemyCasting)
	{
		msg.clientDefending = 2;
		msg.clientDefendTime = static_cast<float>(Max(0.0, m_state.enemyCastDuration - m_state.enemyCastTimer.sF()));
	}
	else if (m_remoteDefending)
	{
		msg.clientDefending = 1;
		msg.clientDefendTime = m_remoteDefending ? static_cast<float>(Max(0.0, m_remoteDefendEndTime - Scene::Time())) : 0.0f;
	}
	else
	{
		msg.clientDefending = 0;
		msg.clientDefendTime = 0.0f;
	}

	// 詠唱スロット番号をreservedフィールドにエンコード
	msg.reserved = static_cast<uint8>(
		((m_state.playerCastingSlot + 1) << 4) | 
		(m_state.enemyCastingSlot + 1)
	);

	m_multiplayer->send(msg);
}

void Game::applyStateSync(const net::StateSnapshotMessage& msg)
{
	m_state.playerHP = msg.clientHP;
	m_state.enemyHP = msg.hostHP;
	m_state.costValue = msg.clientCost;
	
	// クレイジーモード中はゲージを上書きしない
	if (!m_state.playerCrazyMode)
	{
		m_state.playerCrazy = msg.clientCrazy;
	}
	if (!m_state.enemyCrazyMode)
	{
		m_state.enemyCrazy = msg.hostCrazy;
	}

	// クライアント側（自分）の状態デコード
	if (msg.clientDefending == 2)
	{
		// 詠唱中
		if (!m_state.playerCasting)
		{
			// 詠唱開始
			m_state.playerCasting = true;
			m_state.playerCastTimer.restart();
			m_state.playerCastDuration = msg.clientDefendTime;
			m_state.playerCastingSlot = (msg.reserved & 0x0F) - 1;
		}
		else
		{
			// 詠唱継続（残り時間更新）
			double elapsed = m_state.playerCastTimer.sF();
			m_state.playerCastDuration = msg.clientDefendTime + elapsed;
		}
		m_state.defending = false;
	}
	else if (msg.clientDefending == 1)
	{
		// 防御中
		m_state.defending = true;
		m_state.defendTimer.restart();
		if (m_state.playerCasting)
		{
			BattleLogic::cancelCasting(m_state, true);
		}
	}
	else
	{
		// 通常状態
		m_state.defending = false;
		m_state.defendTimer.reset();
		if (m_state.playerCasting)
		{
			BattleLogic::cancelCasting(m_state, true);
		}
	}

	// ホスト側（敵）の状態デコード
	if (msg.hostDefending == 2)
	{
		// 詠唱中
		if (!m_state.enemyCasting)
		{
			// 詠唱開始
			m_state.enemyCasting = true;
			m_state.enemyCastTimer.restart();
			m_state.enemyCastDuration = msg.hostDefendTime;
			m_state.enemyCastingSlot = ((msg.reserved >> 4) & 0x0F) - 1;
		}
		else
		{
			// 詠唱継続
			double elapsed = m_state.enemyCastTimer.sF();
			m_state.enemyCastDuration = msg.hostDefendTime + elapsed;
		}
	}
	else
	{
		// 詠唱終了または防御/通常状態
		if (m_state.enemyCasting)
		{
			BattleLogic::cancelCasting(m_state, false);
		}
	}

	if (!m_isHost)
	{
		m_remoteCostValue = msg.hostCost;
	}
}

void Game::broadcastEventToClient(net::BattleEventType type, int32 primaryValue, int32 secondaryValue, uint32 flags)
{
	if (!(m_isOnlineMode && m_isHost && m_multiplayer && m_multiplayer->isConnected()))
	{
		return;
	}

	net::BattleEventMessage msg{};
	msg.eventType = type;
	msg.primaryValue = primaryValue;
	msg.secondaryValue = secondaryValue;
	msg.flags = flags;

	m_multiplayer->send(msg);
}

void Game::emitLocalEvent(net::BattleEventType type, int32 primaryValue, int32 secondaryValue, uint32 flags)
{
	bool localPerspective = true;
	if (m_isOnlineMode)
	{
		const bool localIsHost = m_isHost;
		switch (type)
		{
		case net::BattleEventType::HostAttackDamage:
		case net::BattleEventType::HostAttackBlocked:
		case net::BattleEventType::HostActionRejected:
		case net::BattleEventType::HostDefend:
		case net::BattleEventType::HostEscape:
			localPerspective = localIsHost;
			break;
		case net::BattleEventType::ClientAttackDamage:
		case net::BattleEventType::ClientAttackBlocked:
		case net::BattleEventType::ClientActionRejected:
		case net::BattleEventType::ClientDefend:
		case net::BattleEventType::ClientEscape:
			localPerspective = !localIsHost;
			break;
		default:
			localPerspective = localIsHost;
			break;
		}
	}

	const String message = renderBattleEvent(type, primaryValue, secondaryValue, flags, localPerspective);
	if (!message.isEmpty())
	{
		pushLog(message);
		m_state.battleMessage = message;
	}

	if (flags & net::EventFlagHitEnemy)
	{
		BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Enemy);
	}
	else if (flags & net::EventFlagHitPlayer)
	{
		BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Player);
	}
}

String Game::renderBattleEvent(net::BattleEventType type, int32 primaryValue, int32 secondaryValue, uint32 flags, bool localPerspective) const
{
	switch (type)
	{
	case net::BattleEventType::HostAttackDamage:
		return localPerspective ? U"攻撃成功！{}ダメージ！"_fmt(primaryValue)
			: U"相手の攻撃！{}ダメージ！"_fmt(primaryValue);
	case net::BattleEventType::HostAttackBlocked:
		return localPerspective ? U"攻撃は防がれた…" : U"相手の攻撃を防いだ！";
	case net::BattleEventType::HostActionRejected:
		switch (primaryValue)
		{
		case 0: return localPerspective ? U"コストが足りない！" : U"相手はコスト不足だ";
		case 1: return localPerspective ? U"防御中は行動できない" : U"相手は行動できなかった";
		case 2: return localPerspective ? U"今はその行動を受け付けていない" : U"相手は不正なタイミングで行動した";
		default: return localPerspective ? U"その行動はできない" : U"相手の行動は無効化された";
		}
	case net::BattleEventType::HostDefend:
		return localPerspective ? U"防御体勢に入った！" : U"相手が防御した";
	case net::BattleEventType::HostEscape:
		return localPerspective ? U"逃走を試みた！" : U"相手が逃げ出そうとしている";
	case net::BattleEventType::ClientAttackDamage:
		return localPerspective ? U"攻撃成功！{}ダメージ！"_fmt(primaryValue)
			: U"相手の攻撃！{}ダメージ！"_fmt(primaryValue);
	case net::BattleEventType::ClientAttackBlocked:
		return localPerspective ? U"攻撃は防がれた…" : U"相手の攻撃を防いだ！";
	case net::BattleEventType::ClientActionRejected:
		switch (primaryValue)
		{
		case 0: return localPerspective ? U"コストが足りない！" : U"相手はコスト不足だ";
		case 1: return localPerspective ? U"防御中は行動できない" : U"相手は行動できなかった";
		case 2: return localPerspective ? U"今はその行動が使えない" : U"相手の行動は無効なタイミングだった";
		default: return localPerspective ? U"その行動はできない" : U"相手の行動は無効化された";
		}
	case net::BattleEventType::ClientDefend:
		return localPerspective ? U"防御体勢に入った！" : U"相手が防御した";
	case net::BattleEventType::ClientEscape:
		return localPerspective ? U"逃走を試みた！" : U"相手が逃げ出そうとしている";
	case net::BattleEventType::TurnChanged:
		return U"";
	default:
		return U"";
	}
}

void Game::handlePlayerAttack(int slotIndex, int32 damage, net::BattleEventType eventType, bool broadcastToClient)
{
	BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Enemy);
	const int32 finalDamage = m_remoteDefending ? 0 : damage;
	m_state.enemyHP = Max(0, m_state.enemyHP - finalDamage);

	// 防御成功時はクレイジーゲージを変更しない
	if (finalDamage > 0)
	{
		BattleLogic::addCrazy(m_state, true, +20);
		BattleLogic::addCrazy(m_state, false, -10);
	}

	m_deck.onUse(slotIndex);

	// クレイジーモード中は実際のカード名を表示
	if (m_deck.isCrazyMode() && slotIndex >= 0)
	{
		const CardSpec& actualCard = m_deck.getActualCard(slotIndex);
		pushLog(U"→ 実際は「" + actualCard.name + U"」が発動！");
	}

	// 防御成功時はAttackBlockedイベントに変更
	net::BattleEventType actualEventType = eventType;
	if (m_remoteDefending)
	{
		if (eventType == net::BattleEventType::HostAttackDamage)
		{
			actualEventType = net::BattleEventType::HostAttackBlocked;
		}
	}

	const uint32 flags = net::EventFlagHitEnemy;
	emitLocalEvent(actualEventType, finalDamage, slotIndex, flags);

	if (broadcastToClient)
	{
		sendStateSync();
		broadcastEventToClient(actualEventType, finalDamage, slotIndex, flags);
	}

	// カード飛翔演出を開始（有効なスロットの場合のみ）
	if (slotIndex >= 0 && slotIndex < static_cast<int>(m_deck.current().size()))
	{
		const CardSpec& visualCard = m_deck.getVisualCard(slotIndex);
		String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
		startPlayerProjectile(slotIndex, cardName);
	}

	replaceUsedCardIfNeeded();
	finishBattleIfNeeded();
}

void Game::handleActionRejected(int reasonCode, net::BattleEventType eventType, bool broadcastToClient)
{
	const uint32 flags = 0;
	emitLocalEvent(eventType, reasonCode, 0, flags);

	if (broadcastToClient)
	{
		broadcastEventToClient(eventType, reasonCode, 0, flags);
	}
}

void Game::handleDefend(net::BattleEventType eventType, bool broadcastToClient)
{
	if (eventType == net::BattleEventType::HostDefend)
	{
		m_state.defending = true;
		m_state.defendTimer.restart();
	}
	else if (eventType == net::BattleEventType::ClientDefend)
	{
		if (!m_isHost)
		{
			m_state.defending = true;
			m_state.defendTimer.restart();
		}
	}

	const uint32 flags = 0;
	emitLocalEvent(eventType, 0, 0, flags);

	if (broadcastToClient)
	{
		sendStateSync();
		broadcastEventToClient(eventType, 0, 0, flags);
	}
}

void Game::handleEscape(net::BattleEventType eventType, bool broadcastToClient)
{
	if (eventType == net::BattleEventType::HostEscape)
	{
		m_state.playerHP = 0;
	}
	else
	{
		m_state.enemyHP = 0;
	}

	const uint32 flags = 0;
	emitLocalEvent(eventType, 0, 0, flags);

	if (broadcastToClient)
	{
		sendStateSync();
		broadcastEventToClient(eventType, 0, 0, flags);
	}

	finishBattleIfNeeded();
}

void Game::handleEnemyAttack(int32 slotIndex, int32 damage, net::BattleEventType eventType, bool broadcastToClient)
{
	const int32 finalDamage = m_state.defending ? 0 : damage;
	m_state.playerHP = Max(0, m_state.playerHP - finalDamage);
	BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Player);
	
	// 防御成功時はクレイジーゲージを変更しない
	if (finalDamage > 0)
	{
		BattleLogic::addCrazy(m_state, false, +20);
	}

	// 防御成功時はAttackBlockedイベントに変更
	net::BattleEventType actualEventType = eventType;
	if (m_state.defending)
	{
		if (eventType == net::BattleEventType::ClientAttackDamage)
		{
			actualEventType = net::BattleEventType::ClientAttackBlocked;
		}
	}

	const uint32 flags = net::EventFlagHitPlayer;
	emitLocalEvent(actualEventType, finalDamage, slotIndex, flags);

	if (broadcastToClient)
	{
		sendStateSync();
		broadcastEventToClient(actualEventType, finalDamage, slotIndex, flags);
	}

	// カード飛翔演出を開始（有効なスロットの場合のみ）
	if (slotIndex >= 0 && slotIndex < static_cast<int>(m_deck.current().size()))
	{
		const CardSpec& visualCard = m_deck.getVisualCard(slotIndex);
		String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
		startEnemyProjectile(slotIndex, cardName);
	}

	replaceUsedCardIfNeeded();
	finishBattleIfNeeded();
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
	
	// 開始位置：攻撃ボタンの中心
	const RoundRect attackBtn = BattleLayout::AttackOptionButton(sceneSize, slotIndex);
	m_playerProjectile.startPos = attackBtn.center();
	
	// 目標位置：敵キャラクターの中心
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
	
	// 開始位置：敵キャラクターの中心
	const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;
	m_enemyProjectile.startPos = enemyPos + Vec2{ entitySize.x * 0.5, entitySize.y * 0.5 };
	
	// 目標位置：プレイヤーキャラクターの中心
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
		
		// 飛翔フェーズ完了時に点滅フェーズに移行
		if (!m_playerProjectile.isBlinking && elapsed >= CardProjectile::FlightDuration)
		{
			m_playerProjectile.isBlinking = true;
		}
		
		// 全体の演出時間が経過したら終了
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
		
		// 飛翔フェーズ完了時に点滅フェーズに移行
		if (!m_enemyProjectile.isBlinking && elapsed >= CardProjectile::FlightDuration)
		{
			m_enemyProjectile.isBlinking = true;
		}
		
		// 全体の演出時間が経過したら終了
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
	const Vec2 enemyPos = BattleLayout::EnemyPos(sceneSize);
	const Size entitySize = BattleLayout::EntitySize;

	const int32 playerHPWidth = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_state.playerHP) / BattleState::MaxHP)));
	const int32 enemyHPWidth = static_cast<int32>(Math::Round(BattleLayout::HPBarWidth * (static_cast<double>(m_state.enemyHP) / BattleState::MaxHP)));

	{
		const ScopedRenderTarget2D rt{ m_sceneRT };
		m_sceneRT.clear(ColorF{ 1.0 });

		const double t = m_state.hitTimer.sF();
		const bool hitPlayer = (m_state.hitTarget == BattleState::HitTarget::Player) && (t < BattleState::HitDuration);
		const bool hitEnemy = (m_state.hitTarget == BattleState::HitTarget::Enemy) && (t < BattleState::HitDuration);
		const double flash = hitPlayer || hitEnemy ? (0.5 + 0.5 * Periodic::Square0_1(30.0)) : 0.0;
		const ColorF playerColor = hitPlayer ? ColorF{ 1.0, 0.95 * flash, 0.95 * flash } : ColorF{ 1.0 };
		const ColorF enemyColor = hitEnemy ? ColorF{ 1.0, 0.85 * flash, 0.85 * flash } : ColorF{ 1.0 };
		const RectF pRect{ playerPos, BattleLayout::EntitySize };
		const RectF eRect{ enemyPos, BattleLayout::EntitySize };
		drawFit(m_texPlayer, pRect, playerColor);
		drawFit(m_texEnemy, eRect, enemyColor);

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

		{
			const Vec2 c = BattleLayout::PlayerCrazyCenter(sceneSize);
			const double ratio = Clamp(m_state.playerCrazy / 100.0, 0.0, 1.0);
			Circle{ c, BattleLayout::CrazyRingRadius }.drawFrame(6, 0, ColorF{ 0.85 });
			const double angle = Math::TwoPiF * ratio;
			Circle{ c, BattleLayout::CrazyRingRadius }.drawArc(-Math::HalfPi, angle, 6, 0, ColorF{ 0.2, 0.6, 1.0 });
			const s3d::Texture& face = m_faces.select(m_state.playerCrazy);
			const double s = 26.0;
			face.scaled(s / face.height()).drawAt(c);
		}

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

		const RoundRect attackBtn1 = BattleLayout::AttackOptionButton(sceneSize, 0);
		const RoundRect attackBtn2 = BattleLayout::AttackOptionButton(sceneSize, 1);
		const RoundRect attackBtn3 = BattleLayout::AttackOptionButton(sceneSize, 2);
		const RoundRect attackBtn4 = BattleLayout::AttackOptionButton(sceneSize, 3);
		const RoundRect escapeBtn = BattleLayout::EscapeButton(sceneSize);
		const ColorF actionBg{ 1.0 };
		const bool disabledAll = m_state.defending;

		auto drawSlot = [&](const RoundRect& rr, int slot)
		{
			const bool isRefilling = m_deck.isSlotRefilling(slot);
			
			if (isRefilling)
			{
				// クールタイム中：円形プログレスを表示
				rr.draw(ColorF{ 0.95 }).drawFrame(2);
				
				const double progress = m_deck.getSlotRefillProgress(slot);
				const Vec2 center = rr.center();
				const double radius = 30.0;
				
				// 背景円
				Circle{ center, radius }.drawFrame(4, ColorF{ 0.7 });
				
				// プログレス円（上から時計回りに描画）
				const double angle = Math::TwoPi * progress;
				Circle{ center, radius }.drawArc(-Math::HalfPi, angle, 4, 0, ColorF{ 0.2, 0.6, 1.0 });
				
				// 残り時間テキスト
				const double remainingSec = (1.0 - progress) * CardDeck::RefillCooldownSec;
				FontAsset(U"Bold")(U"{:.1f}"_fmt(remainingSec)).drawAt(18, center, ColorF{ 0.3 });
			}
			else
			{
				// 通常：カードを表示
				const bool hasCurrent = (slot < static_cast<int>(m_deck.current().size()));
				const bool hasLast = (!hasCurrent && (slot < static_cast<int>(m_deck.lastDisplayed().size())));
				const bool hasAny = hasCurrent || hasLast;
				const ColorF base = disabledAll || !hasAny ? ColorF{ 0.95 } : actionBg;
				rr.draw(base).drawFrame(2);
				String title;
				if (hasAny)
				{
					const CardSpec& c = hasCurrent ? m_deck.getVisualCard(slot) : m_deck.lastDisplayed()[slot];
					title = (c.name.isEmpty() ? U"攻撃{}"_fmt(slot + 1) : c.name);
				}
				else
				{
					title = U"攻撃{}"_fmt(slot + 1);
				}
				const ColorF txt = disabledAll || !hasAny ? ColorF{ 0.5 } : ColorF{ 0.1 };
				FontAsset(U"Bold")(title).drawAt(20, rr.center(), txt);
			}
		};

		drawSlot(attackBtn1, 0);
		drawSlot(attackBtn2, 1);
		drawSlot(attackBtn3, 2);
		drawSlot(attackBtn4, 3);

		escapeBtn.draw(ColorF{ 1.0, m_escapeTr.value() });
		BaseFrame().draw(escapeBtn.rect);
		FontAsset(U"Bold")(U"逃走").drawAt(24, escapeBtn.center(), ColorF{ 0.1 });

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

		const RectF playerPanel = BattleLayout::PlayerPanelRect(sceneSize);
		const RoundRect panelRR{ playerPanel, BattleLayout::PlayerPanelR };
		panelRR.draw(ColorF{ 1.0, 0.95 });
		BaseFrame().draw(panelRR.rect);
		BattleLayout::PlayerIconRect(playerPanel).rounded(6).draw(ColorF{ 0.3, 0.7, 0.9 });
		const RoundRect defendBtn = BattleLayout::DefendButtonRect(playerPanel);
		defendBtn.draw(ColorF{ 1.0 }).drawFrame(2);
		FontAsset(U"Bold")(U"防御").drawAt(24, defendBtn.center(), ColorF{ 0.1 });

		if (!m_eventLog.isEmpty())
		{
			const double panelMargin = 20.0;
			const double lineHeight = 22.0;
			const size_t displayCount = m_eventLog.size();
			const double panelWidth = sceneSize.x - (panelMargin * 2.0);
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
				const double age = now - entry.timestamp;
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

		// ===== 詠唱ゲージ =====
		// プレイヤーの詠唱ゲージ
		if (m_state.playerCasting)
		{
			const double progress = BattleLogic::getCastingProgress(m_state, true);
			const Vec2 gaugePos{ 100, 500 };
			const double gaugeWidth = 200;
			const double gaugeHeight = 20;
			
			// ゲージ背景
			RectF{ gaugePos, gaugeWidth, gaugeHeight }.draw(ColorF{ 0.2, 0.2, 0.2 });
			// ゲージ前景
			RectF{ gaugePos, gaugeWidth * progress, gaugeHeight }.draw(ColorF{ 0.8, 0.6, 0.2 });
			// テキスト
			FontAsset(U"Bold")(U"詠唱中: " + m_state.playerCastingCardName)
				.draw(24, Vec2{ gaugePos.x + 5, gaugePos.y - 25 }, ColorF{ 1.0 });
			// 残り時間
			const double remainingTime = m_state.playerCastDuration - m_state.playerCastTimer.sF();
			FontAsset(U"Bold")(U"{:.1f}秒"_fmt(remainingTime))
				.draw(20, Vec2{ gaugePos.x + gaugeWidth + 10, gaugePos.y + 2 }, ColorF{ 1.0 });
		}
		
		// 敵の詠唱中表示（ゲージは非表示）
		if (m_state.enemyCasting)
		{
			const Vec2 textPos{ 500, 100 };
			// テキストのみ表示
			FontAsset(U"Bold")(U"詠唱中")
				.draw(24, textPos, ColorF{ 1.0, 0.5, 0.5 });
		}

		// ===== カード飛翔演出の描画 =====
		auto drawProjectile = [&](const CardProjectile& proj)
		{
			if (!proj.active) return;
			
			const double elapsed = proj.timer.sF();
			
			// 点滅フェーズの処理
			if (proj.isBlinking)
			{
				// 点滅開始からの経過時間
				const double blinkElapsed = elapsed - CardProjectile::FlightDuration;
				const double blinkProgress = blinkElapsed / CardProjectile::BlinkDuration;
				
				// 点滅効果（10Hzで点滅）
				const bool isVisible = (static_cast<int>(blinkElapsed * 10.0) % 2) == 0;
				
				if (isVisible)
				{
					// 目標位置に固定
					const Vec2 currentPos = proj.targetPos;
					
					// カードのサイズ（点滅中は70%のサイズ）
					const double cardSize = 80.0 * 0.7;
					const RoundRect cardRect{ Arg::center = currentPos, cardSize, cardSize * 0.6, 8.0 };
					
					// 点滅時の色（白っぽく光る）
					const double flashIntensity = 1.0 - blinkProgress * 0.3;
					cardRect.draw(ColorF{ 1.0, 1.0, 0.9, flashIntensity }).drawFrame(2, ColorF{ 1.0, 0.8, 0.2, flashIntensity });
					
					// カード名を描画
					const double fontSize = 14.0;
					FontAsset(U"Bold")(proj.cardName).drawAt(fontSize, currentPos, ColorF{ 0.1, 0.1, 0.2, flashIntensity });
				}
			}
			else
			{
				// 飛翔フェーズの処理
				const double t = Clamp(elapsed / CardProjectile::FlightDuration, 0.0, 1.0);
				
				// イージング（加速→減速）
				const double eased = EaseInOutQuad(t);
				
				// 現在の位置を計算
				const Vec2 currentPos = proj.startPos.lerp(proj.targetPos, eased);
				
				// カードのサイズ（飛んでいる間は少し小さく）
				const double cardSize = 80.0 * (1.0 - 0.3 * t);
				const RoundRect cardRect{ Arg::center = currentPos, cardSize, cardSize * 0.6, 8.0 };
				
				// カードの背景を描画
				cardRect.draw(ColorF{ 1.0, 1.0, 0.9, 0.95 - 0.3 * t }).drawFrame(2, ColorF{ 0.2, 0.2, 0.3 });
				
				// カード名を描画
				const double fontSize = 16.0 * (1.0 - 0.2 * t);
				FontAsset(U"Bold")(proj.cardName).drawAt(fontSize, currentPos, ColorF{ 0.1, 0.1, 0.2, 0.9 - 0.4 * t });
				
				// 軌跡エフェクト（残像）
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
		
		// プレイヤーの飛翔演出を描画
		drawProjectile(m_playerProjectile);
		
		// 敵の飛翔演出を描画
		drawProjectile(m_enemyProjectile);

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
	ScreenFrame().draw(s3d::RectF{ 0, 0, static_cast<double>(Scene::Width()), static_cast<double>(Scene::Height()) });
}
