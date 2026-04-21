#include "../scenes/Game_Impl.hpp"
#include "BattleLogic.hpp"
#include "BattleUtils.hpp"

// =============================================================================
//  Game::PvELoop  ─ シングルプレイ（プレイヤー vs AI）のバトルループ
// =============================================================================
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
			g.triggerCrazyZoom(true);
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

// =============================================================================
//  Game::PvPHostLoop  ─ PvP ホスト側のバトルループ
// =============================================================================
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
			g.triggerCrazyZoom(true);
			g.sendStateSync();
		}
		// クライアント側のクレイジーモード発動チェック（ホストが権威として管理）
		if (BattleLogic::shouldEnterCrazyMode(g.m_state, false))
		{
			BattleLogic::startCrazyMode(g.m_state, false, Scene::Time());
			g.triggerCrazyZoom(false);
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
			// クライアントのリクエスト受信時に実カードから計算済みのダメージを使用
			// 未設定の場合のみホストのデッキでフォールバック計算する
			const int32 damage = (m_enemyPendingDamage > 0)
				? m_enemyPendingDamage
				: BattleUtils::slotDamage(slot, g.m_deck);
			g.handleEnemyAttack(slot, damage, net::BattleEventType::ClientAttackDamage, true);
			BattleLogic::cancelCasting(g.m_state, false);
			m_enemyPendingDamage = 0;
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
			BattleLogic::startCasting(g.m_state, true, slotIndex, visualCard.name, visualCard.delaySec);
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
			else if (type == net::PacketType::ClientHandSync)
			{
				auto msg = net.receive<net::ClientHandSyncMessage>();
				if (!msg)
				{
					break;
				}
				// クライアントの手札テーブルを更新
				for (int i = 0; i < 4; ++i)
				{
					m_game.m_clientActualHand[static_cast<size_t>(i)] = msg->actualIndices[i];
					m_game.m_clientVisualHand[static_cast<size_t>(i)] = msg->visualIndices[i];
				}
				if (g_networkLog.is_open()) {
					g_networkLog << "ClientHandSync received:"
								<< " actual=[" << static_cast<int>(msg->actualIndices[0])
								<< "," << static_cast<int>(msg->actualIndices[1])
								<< "," << static_cast<int>(msg->actualIndices[2])
								<< "," << static_cast<int>(msg->actualIndices[3]) << "]"
								<< " visual=[" << static_cast<int>(msg->visualIndices[0])
								<< "," << static_cast<int>(msg->visualIndices[1])
								<< "," << static_cast<int>(msg->visualIndices[2])
								<< "," << static_cast<int>(msg->visualIndices[3]) << "]"
								<< std::endl;
				}
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

			// カード特定の優先順位:
			// 1. ホストが管理するクライアント手札テーブル (m_clientActualHand / m_clientVisualHand)
			// 2. メッセージに含まれるプールインデックス
			// 3. ホスト自身のデッキ（最終フォールバック）
			const CardSpec* clientVisualCard = nullptr;
			const CardSpec* clientActualCard = nullptr;

			if (slotIndex >= 0 && slotIndex < 4)
			{
				clientVisualCard = g.m_deck.getCardByPoolIndex(g.m_clientVisualHand[static_cast<size_t>(slotIndex)]);
				clientActualCard = g.m_deck.getCardByPoolIndex(g.m_clientActualHand[static_cast<size_t>(slotIndex)]);
			}
			if (!clientVisualCard)
			{
				clientVisualCard = g.m_deck.getCardByPoolIndex(msg.visualCardPoolIndex);
			}
			if (!clientActualCard)
			{
				clientActualCard = g.m_deck.getCardByPoolIndex(msg.actualCardPoolIndex);
			}
			if (!clientVisualCard && slotIndex < static_cast<int>(cards.size()))
			{
				clientVisualCard = &g.m_deck.getVisualCard(slotIndex);
			}
			if (!clientActualCard && slotIndex < static_cast<int>(cards.size()))
			{
				clientActualCard = &g.m_deck.getActualCard(slotIndex);
			}
			if (!clientVisualCard)
			{
				g.handleActionRejected(3, net::BattleEventType::ClientActionRejected, true);
				return;
			}

			// クライアントの視覚カードのコストで検証
			const double cost = clientVisualCard->cost;
			if (g.remoteAvailableCost() < cost)
			{
				g.handleActionRejected(0, net::BattleEventType::ClientActionRejected, true);
				return;
			}

			g.consumeRemoteCost(cost);

			// 詠唱開始（クライアント側）
			if (slotIndex >= 0 && slotIndex < static_cast<int>(cards.size()))
			{
				// 視覚カードの delaySec を詠唱時間として使用（クライアントと一致させる）
				BattleLogic::startCasting(g.m_state, false, slotIndex, clientVisualCard->name, clientVisualCard->delaySec);

				// 詠唱完了時に使うダメージを実カードの名前から事前計算して保持
				// （属性IDはhandleEnemyAttack内でキャスト完了時に更新する）
				const CardSpec* damageCard = clientActualCard ? clientActualCard : clientVisualCard;
				m_enemyPendingDamage = BattleUtils::calculateDamage(damageCard->name);

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
	int32 m_enemyPendingDamage = 0; // 詠唱完了時に使うクライアント側の事前計算ダメージ
};

// =============================================================================
//  Game::PvPClientLoop  ─ PvP クライアント側のバトルループ
// =============================================================================
class Game::PvPClientLoop : public Game::BattleLoop
{
public:
	using Game::BattleLoop::BattleLoop;

	void onEnter() override
	{
		// ゲーム開始時に初期手札をホストへ送信
		sendHandSync();
	}

	void update(const BattleInput& input) override
	{
		Game& g = m_game;

		processIncomingPackets();

		// 手札の変化を検出してホストへ同期
		checkAndSendHandSync();

		// クレイジーモード発動はホストが権威として管理し applyStateSync 経由で反映される

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

		// 攻撃リクエスト時は選択カードの属性ID・プールインデックスを付与
		if (slotIndex >= 0 && slotIndex < static_cast<int>(m_game.m_deck.current().size()))
		{
			const CardSpec& visualCard = m_game.m_deck.getVisualCard(slotIndex);
			msg.cardAttributeId = static_cast<uint8>(visualCard.attributeId);

			const int32 visIdx = m_game.m_deck.getVisualCardPoolIndex(slotIndex);
			const int32 actIdx = m_game.m_deck.getActualCardPoolIndex(slotIndex);
			msg.visualCardPoolIndex = (visIdx >= 0 && visIdx < 0xFF) ? static_cast<uint8>(visIdx) : 0xFF;
			msg.actualCardPoolIndex = (actIdx >= 0 && actIdx < 0xFF) ? static_cast<uint8>(actIdx) : 0xFF;
		}

		if (g_networkLog.is_open()) {
			g_networkLog << "sendRequest:"
						<< " action=" << static_cast<int>(action)
						<< " slotIndex=" << slotIndex
						<< " cardAttributeId=" << static_cast<int>(msg.cardAttributeId)
						<< " visualCardPoolIndex=" << static_cast<int>(msg.visualCardPoolIndex)
						<< " actualCardPoolIndex=" << static_cast<int>(msg.actualCardPoolIndex)
						<< " playerAttributeId=" << m_game.m_state.playerAttributeId
						<< " enemyAttributeId=" << m_game.m_state.enemyAttributeId
						<< std::endl;
		}

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
				// クライアント側で自分の攻撃イベントを受信した場合、カードを更新＆演出開始
				if (msg->eventType == net::BattleEventType::ClientAttackDamage ||
				    msg->eventType == net::BattleEventType::ClientAttackBlocked)
				{
					const bool isBlocked = (msg->eventType == net::BattleEventType::ClientAttackBlocked);
					int32 slotIndex = msg->secondaryValue;
					bool projectileStarted = false;
					if (slotIndex >= 0 && slotIndex < 4)
					{
						// 攻撃アニメーション開始
						m_game.m_playerAttackAnimActive = true;
						m_game.m_playerAttackAnimTimer.restart();

						m_game.m_deck.onUse(slotIndex);
						m_game.replaceUsedCardIfNeeded();

						// カード飛翔演出を開始し、到達時に演出を遅延適用
						if (slotIndex < static_cast<int>(m_game.m_deck.current().size()))
						{
							const CardSpec& visualCard = m_game.m_deck.getVisualCard(slotIndex);
							String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
							m_game.startPlayerProjectile(slotIndex, cardName, isBlocked);

							auto& impact = m_game.m_playerProjectile.pendingImpact;
							impact.valid          = true;
							impact.eventType      = msg->eventType;
							impact.primaryValue   = msg->primaryValue;
							impact.secondaryValue = msg->secondaryValue;
							impact.flags          = msg->flags;
							impact.shouldCheckBattleEnd = false;
							impact.shouldSendStateSync  = false;
							projectileStarted = true;
						}
					}
					// プロジェクタイルが起動できない場合は即時フラッシュ
					if (!projectileStarted)
						m_game.emitLocalEvent(msg->eventType, msg->primaryValue, msg->secondaryValue, msg->flags);
				}
				// クライアント側でホストの攻撃イベントを受信した場合、敵側の演出開始
				else if (msg->eventType == net::BattleEventType::HostAttackDamage ||
				         msg->eventType == net::BattleEventType::HostAttackBlocked)
				{
					const bool isBlocked = (msg->eventType == net::BattleEventType::HostAttackBlocked);
					int32 slotIndex = msg->secondaryValue;
					bool projectileStarted = false;

					// 敵攻撃アニメーション開始
					m_game.m_enemyAttackAnimActive = true;
					m_game.m_enemyAttackAnimTimer.restart();

					// 敵からの攻撃演出を開始し、到達時に演出を遅延適用
					if (slotIndex >= 0 && slotIndex < static_cast<int>(m_game.m_deck.current().size()))
					{
						const CardSpec& visualCard = m_game.m_deck.getVisualCard(slotIndex);
						String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
						m_game.startEnemyProjectile(slotIndex, cardName, isBlocked);

						auto& impact = m_game.m_enemyProjectile.pendingImpact;
						impact.valid          = true;
						impact.eventType      = msg->eventType;
						impact.primaryValue   = msg->primaryValue;
						impact.secondaryValue = msg->secondaryValue;
						impact.flags          = msg->flags;
						impact.shouldCheckBattleEnd = false;
						impact.shouldSendStateSync  = false;
						projectileStarted = true;
					}
					// プロジェクタイルが起動できない場合は即時フラッシュ
					if (!projectileStarted)
						m_game.emitLocalEvent(msg->eventType, msg->primaryValue, msg->secondaryValue, msg->flags);
				}
				// クライアント側で自分の防御イベントを受信した場合、防御状態を設定
				else if (msg->eventType == net::BattleEventType::ClientDefend)
				{
					m_game.m_state.defending = true;
					m_game.m_state.defendTimer.restart();
					m_game.emitLocalEvent(msg->eventType, msg->primaryValue, msg->secondaryValue, msg->flags);
				}
				// 攻撃・防御以外のイベントは即時適用
				else
				{
					m_game.emitLocalEvent(msg->eventType, msg->primaryValue, msg->secondaryValue, msg->flags);
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

	// 手札の変化を検出し、変化があればホストへ同期パケットを送る
	void checkAndSendHandSync()
	{
		if (!m_game.m_multiplayer || m_game.m_multiplayer->getConnectionState() != MultiplayerManager::ConnectionState::Connected)
		{
			return;
		}

		const auto& cards = m_game.m_deck.current();
		for (int i = 0; i < 4; ++i)
		{
			// リフィル待ちのスロットはカードが確定していないのでスキップ
			if (m_game.m_deck.isSlotRefilling(i)) continue;

			uint8 currentIdx = 0xFF;
			if (i < static_cast<int>(cards.size()))
			{
				const int32 actIdx = m_game.m_deck.getActualCardPoolIndex(i);
				currentIdx = (actIdx >= 0 && actIdx < 0xFF) ? static_cast<uint8>(actIdx) : 0xFF;
			}

			if (currentIdx != m_lastSentActualHand[static_cast<size_t>(i)])
			{
				sendHandSync();
				return;
			}
		}
	}

	// 現在の手札状態をホストへ送信し、送信済みテーブルを更新する
	void sendHandSync()
	{
		if (!m_game.m_multiplayer || m_game.m_multiplayer->getConnectionState() != MultiplayerManager::ConnectionState::Connected)
		{
			return;
		}

		net::ClientHandSyncMessage msg{};
		const auto& cards = m_game.m_deck.current();
		for (int i = 0; i < 4; ++i)
		{
			if (i < static_cast<int>(cards.size()))
			{
				const int32 actIdx = m_game.m_deck.getActualCardPoolIndex(i);
				const int32 visIdx = m_game.m_deck.getVisualCardPoolIndex(i);
				msg.actualIndices[i] = (actIdx >= 0 && actIdx < 0xFF) ? static_cast<uint8>(actIdx) : 0xFF;
				msg.visualIndices[i] = (visIdx >= 0 && visIdx < 0xFF) ? static_cast<uint8>(visIdx) : 0xFF;
			}
		}

		m_game.m_multiplayer->send(msg);

		// 送信済みテーブルを更新
		for (int i = 0; i < 4; ++i)
		{
			m_lastSentActualHand[static_cast<size_t>(i)] = msg.actualIndices[i];
		}

		if (g_networkLog.is_open()) {
			g_networkLog << "ClientHandSync sent:"
						<< " actual=[" << static_cast<int>(msg.actualIndices[0])
						<< "," << static_cast<int>(msg.actualIndices[1])
						<< "," << static_cast<int>(msg.actualIndices[2])
						<< "," << static_cast<int>(msg.actualIndices[3]) << "]"
						<< " visual=[" << static_cast<int>(msg.visualIndices[0])
						<< "," << static_cast<int>(msg.visualIndices[1])
						<< "," << static_cast<int>(msg.visualIndices[2])
						<< "," << static_cast<int>(msg.visualIndices[3]) << "]"
						<< std::endl;
		}
	}

	uint32 m_nextRequestId = 1;
	// 直近にホストへ送信した手札の実カードインデックス (変化検出用, 0xFF=未送信)
	std::array<uint8, 4> m_lastSentActualHand{0xFF, 0xFF, 0xFF, 0xFF};
};

// =============================================================================
//  setupBattleLoop  ─ ループクラスの完全定義が見えるここで実体化する
// =============================================================================
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
