#include "../scenes/Game_Impl.hpp"
#include "../game/BattleLogic.hpp"

// =============================================================================
//  ネットワーク同期・通信処理
// =============================================================================

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
	// isHostTurn フィールドをクレイジーモードフラグとして転用
	// bit0 = ホストのクレイジーモード, bit1 = クライアントのクレイジーモード
	msg.isHostTurn = static_cast<uint8>(
		(m_state.playerCrazyMode ? 0x01 : 0x00) |
		(m_state.enemyCrazyMode  ? 0x02 : 0x00)
	);
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

	// 属性IDを同期
	msg.hostAttributeId = m_state.playerAttributeId;
	msg.clientAttributeId = m_state.enemyAttributeId;

	if (g_networkLog.is_open()) {
		g_networkLog << "sendStateSync:"
					<< " hostAttributeId=" << msg.hostAttributeId
					<< " clientAttributeId=" << msg.clientAttributeId
					<< " playerAttributeId(state)=" << m_state.playerAttributeId
					<< " enemyAttributeId(state)=" << m_state.enemyAttributeId
					<< std::endl;
	}

	m_multiplayer->send(msg);
}

void Game::applyStateSync(const net::StateSnapshotMessage& msg)
{
	m_state.playerHP = msg.clientHP;
	m_state.enemyHP = msg.hostHP;
	m_state.costValue = msg.clientCost;

	// クレイジーモードフラグを受信して反映
	// ホスト視点: bit0=host, bit1=client → クライアント視点: bit0=enemy, bit1=self
	const bool hostCrazyMode   = (msg.isHostTurn & 0x01) != 0;
	const bool clientCrazyMode = (msg.isHostTurn & 0x02) != 0;

	// 自分（クライアント）のクレイジーモード
	if (clientCrazyMode && !m_state.playerCrazyMode)
	{
		BattleLogic::startCrazyMode(m_state, true, Scene::Time());
		m_deck.enterCrazyMode();
		pushLog(U"【CRAZY MODE 発動！】");
		triggerCrazyZoom(true);
	}
	else if (!clientCrazyMode && m_state.playerCrazyMode)
	{
		BattleLogic::endCrazyMode(m_state, true);
		m_deck.exitCrazyMode();
		pushLog(U"【CRAZY MODE 終了】");
	}

	// ホスト（敵）のクレイジーモード
	if (hostCrazyMode && !m_state.enemyCrazyMode)
	{
		BattleLogic::startCrazyMode(m_state, false, Scene::Time());
		triggerCrazyZoom(false);
	}
	else if (!hostCrazyMode && m_state.enemyCrazyMode)
	{
		BattleLogic::endCrazyMode(m_state, false);
	}

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

	// 属性同期（ホスト→敵、クライアント→自分として受信）
	// BattleEvent で既に設定済みの場合は上書きしない（StateSync が古い値で戻すのを防ぐ）
	if (msg.clientAttributeId != 0 || m_state.playerAttributeId == 0)
		m_state.playerAttributeId = msg.clientAttributeId;
	if (msg.hostAttributeId != 0 || m_state.enemyAttributeId == 0)
		m_state.enemyAttributeId = msg.hostAttributeId;
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
