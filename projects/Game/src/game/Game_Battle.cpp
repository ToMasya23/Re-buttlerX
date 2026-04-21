#include "../scenes/Game_Impl.hpp"
#include "BattleLogic.hpp"
#include "BattleUtils.hpp"

// =============================================================================
//  バトルイベント処理（攻撃・防御・逃走・リジェクト・ローカルイベント発行）
// =============================================================================

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
	else
	{
		// PvE: Client* イベントは敵側の行動なので localPerspective = false
		switch (type)
		{
		case net::BattleEventType::ClientAttackDamage:
		case net::BattleEventType::ClientAttackBlocked:
		case net::BattleEventType::ClientActionRejected:
		case net::BattleEventType::ClientDefend:
		case net::BattleEventType::ClientEscape:
			localPerspective = false;
			break;
		default:
			localPerspective = true;
			break;
		}
	}

	const String message = renderBattleEvent(type, primaryValue, secondaryValue, flags, localPerspective);
	if (!message.isEmpty())
	{
		pushLog(message);
		m_state.battleMessage = message;
	}

	// クライアント側では BattleState の player/enemy が逆転しているため HitTarget を入れ替える
	// ホスト: HitEnemy=クライアントが被弾, HitPlayer=ホストが被弾
	// クライアント: playerHP=自分, enemyHP=ホスト → Enemy/Player を反転して適用
	{
		const bool hitEnemy  = (flags & net::EventFlagHitEnemy)  != 0;
		const bool hitPlayer = (flags & net::EventFlagHitPlayer) != 0;
		const bool swapTarget = m_isOnlineMode && !m_isHost;

		if ((hitEnemy && !swapTarget) || (hitPlayer && swapTarget))
			BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Enemy);
		else if ((hitPlayer && !swapTarget) || (hitEnemy && swapTarget))
			BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Player);

		m_state.hitIsWeakness = false; // 後続の属性判定で上書きされる
	}

	// 攻撃イベントに埋め込まれた属性IDを復元して状態に反映
	switch (type)
	{
	case net::BattleEventType::HostAttackDamage:
	case net::BattleEventType::HostAttackBlocked:
	case net::BattleEventType::ClientAttackDamage:
	case net::BattleEventType::ClientAttackBlocked:
	{
		const int32 attrId = net::unpackAttributeId(flags);

		// 属性相性ログ（防御側の現在属性と攻撃属性IDで判定）
		// localPerspective=true → 自分が攻撃側, localPerspective=false → 自分が防御側
		const int32 defenderAttr = localPerspective ? m_state.enemyAttributeId : m_state.playerAttributeId;
		const double mult = Attribute::typeMultiplier(attrId, defenderAttr);
		if (mult > 1.0)
		{
			pushLog(localPerspective
				? U"弱点をついた！ダメージ×{:.1f}！"_fmt(mult)
				: U"弱点をつかれた！ダメージ×{:.1f}！"_fmt(mult));
			// 弱点被弾シェイク：防御側キャラクターを揺らす
			// localPerspective=true → 相手（Enemy）が弱点を突かれた
			// localPerspective=false → 自分（Player）が弱点を突かれた
			triggerWeaknessShake(!localPerspective);
			m_state.hitIsWeakness = true;
		}

		if (localPerspective)
			m_state.playerAttributeId = attrId;
		else
			m_state.enemyAttributeId = attrId;
		break;
	}
	default:
		break;
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
		return U""; // ターン変更メッセージは非表示
	default:
		return U"";
	}
}

void Game::handlePlayerAttack(int slotIndex, int32 damage, net::BattleEventType eventType, bool broadcastToClient)
{
	// 攻撃アニメーション開始
	m_playerAttackAnimActive = true;
	m_playerAttackAnimTimer.restart();

	// 属性相性補正（攻撃側の属性 vs 防御側の現在属性）
	int32 attackerAttr = 0;
	if (slotIndex >= 0 && slotIndex < static_cast<int>(m_deck.current().size()))
		attackerAttr = m_deck.getActualCard(slotIndex).attributeId;
	const double typeMult = Attribute::typeMultiplier(attackerAttr, m_state.enemyAttributeId);
	const int32 boostedDamage = static_cast<int32>(damage * typeMult);

	const int32 finalDamage = m_remoteDefending ? 0 : boostedDamage;

	// 防御成功時はクレイジーゲージ増加なし
	int32 crazyGain = 0;
	if (finalDamage > 0 && slotIndex >= 0 && slotIndex < static_cast<int>(m_deck.current().size()))
		crazyGain = m_deck.getActualCard(slotIndex).addCrazy;

	m_deck.onUse(slotIndex);

	// カード使用後に属性を更新
	if (slotIndex >= 0 && slotIndex < static_cast<int>(m_deck.current().size()))
		m_state.playerAttributeId = m_deck.getActualCard(slotIndex).attributeId;

	// クレイジーモード中は実際のカード名を即時表示
	if (m_deck.isCrazyMode() && slotIndex >= 0)
	{
		const CardSpec& actualCard = m_deck.getActualCard(slotIndex);
		pushLog(U"→ 実際は「" + actualCard.name + U"」が発動！");
	}

	// 防御成功時はAttackBlockedイベントに変更
	net::BattleEventType actualEventType = eventType;
	if (m_remoteDefending && eventType == net::BattleEventType::HostAttackDamage)
		actualEventType = net::BattleEventType::HostAttackBlocked;

	const uint32 flags = net::packAttributeId(net::EventFlagHitEnemy, m_state.playerAttributeId);

	// クライアントへの通知は即時送信（クライアント側の飛翔演出開始に必要）
	if (broadcastToClient)
		broadcastEventToClient(actualEventType, finalDamage, slotIndex, flags);

	// カード飛翔演出を開始し、到達時に適用するダメージ情報を登録
	if (slotIndex >= 0 && slotIndex < static_cast<int>(m_deck.current().size()))
	{
		const CardSpec& visualCard = m_deck.getVisualCard(slotIndex);
		String cardName = visualCard.name.isEmpty() ? U"攻撃{}"_fmt(slotIndex + 1) : visualCard.name;
		startPlayerProjectile(slotIndex, cardName, m_remoteDefending);

		auto& impact = m_playerProjectile.pendingImpact;
		impact.valid               = true;
		impact.hpChange            = finalDamage;
		impact.targetIsPlayer      = false;       // 敵がダメージを受ける
		impact.crazyGain           = crazyGain;
		impact.crazyTargetIsEnemy  = true;
		impact.eventType           = actualEventType;
		impact.primaryValue        = finalDamage;
		impact.secondaryValue      = slotIndex;
		impact.flags               = flags;
		impact.shouldSendStateSync  = broadcastToClient;
		impact.shouldCheckBattleEnd = true;
	}

	replaceUsedCardIfNeeded();
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
	// 敵攻撃アニメーション開始
	m_enemyAttackAnimActive = true;
	m_enemyAttackAnimTimer.restart();

	// 敵の属性を更新（ホスト側のクライアント手札テーブルを優先、なければホストデッキでフォールバック）
	int32 enemyAttackerAttr = 0;
	if (slotIndex >= 0 && slotIndex < 4)
	{
		const CardSpec* clientCard = m_deck.getCardByPoolIndex(m_clientActualHand[static_cast<size_t>(slotIndex)]);
		if (clientCard)
		{
			enemyAttackerAttr = clientCard->attributeId;
			m_state.enemyAttributeId = clientCard->attributeId;
		}
		else if (slotIndex < static_cast<int>(m_deck.current().size()))
		{
			enemyAttackerAttr = m_deck.getActualCard(slotIndex).attributeId;
			m_state.enemyAttributeId = enemyAttackerAttr;
		}
	}

	// 属性相性補正
	const double enemyTypeMult = Attribute::typeMultiplier(enemyAttackerAttr, m_state.playerAttributeId);
	const int32 boostedEnemyDamage = static_cast<int32>(damage * enemyTypeMult);
	const int32 finalDamage = m_state.defending ? 0 : boostedEnemyDamage;

	// 防御成功時はクレイジーゲージ増加なし
	int32 crazyGain = 0;
	if (finalDamage > 0 && slotIndex >= 0 && slotIndex < 4)
	{
		const CardSpec* clientCard = m_deck.getCardByPoolIndex(m_clientActualHand[static_cast<size_t>(slotIndex)]);
		if (clientCard)
			crazyGain = clientCard->addCrazy;
		else if (slotIndex < static_cast<int>(m_deck.current().size()))
			crazyGain = m_deck.getActualCard(slotIndex).addCrazy;
	}

	// 防御成功時はAttackBlockedイベントに変更
	net::BattleEventType actualEventType = eventType;
	if (m_state.defending && eventType == net::BattleEventType::ClientAttackDamage)
		actualEventType = net::BattleEventType::ClientAttackBlocked;

	const uint32 flags = net::packAttributeId(net::EventFlagHitPlayer, m_state.enemyAttributeId);

	// クライアントへの通知は即時送信
	if (broadcastToClient)
		broadcastEventToClient(actualEventType, finalDamage, slotIndex, flags);

	// カード飛翔演出を開始し、到達時に適用するダメージ情報を登録
	if (slotIndex >= 0)
	{
		String cardName;
		if (!m_state.enemyCastingCardName.isEmpty())
			cardName = m_state.enemyCastingCardName;
		else if (slotIndex < 4)
		{
			const CardSpec* clientCard = m_deck.getCardByPoolIndex(m_clientVisualHand[static_cast<size_t>(slotIndex)]);
			if (clientCard && !clientCard->name.isEmpty())
				cardName = clientCard->name;
		}
		if (cardName.isEmpty() && slotIndex < static_cast<int>(m_deck.current().size()))
			cardName = m_deck.getVisualCard(slotIndex).name;
		if (cardName.isEmpty())
			cardName = U"攻撃{}"_fmt(slotIndex + 1);

		startEnemyProjectile(slotIndex, cardName, m_state.defending);

		auto& impact = m_enemyProjectile.pendingImpact;
		impact.valid               = true;
		impact.hpChange            = finalDamage;
		impact.targetIsPlayer      = true;        // プレイヤーがダメージを受ける
		impact.crazyGain           = crazyGain;
		impact.crazyTargetIsEnemy  = false;
		impact.eventType           = actualEventType;
		impact.primaryValue        = finalDamage;
		impact.secondaryValue      = slotIndex;
		impact.flags               = flags;
		impact.shouldSendStateSync  = broadcastToClient;
		impact.shouldCheckBattleEnd = true;
	}
	else
	{
		// slotIndex < 0（PvE即時カウンターなど）：プロジェクタイルなしのため即時適用
		m_state.playerHP = Max(0, m_state.playerHP - finalDamage);
		if (crazyGain > 0)
			BattleLogic::addCrazy(m_state, false, crazyGain);
		BattleLogic::startHitEffect(m_state, BattleState::HitTarget::Player);
		emitLocalEvent(actualEventType, finalDamage, slotIndex, flags);
		if (broadcastToClient)
			sendStateSync();
		finishBattleIfNeeded();
	}

	replaceUsedCardIfNeeded();
}
