#pragma once
#include <memory>
#include <array>
#include "../Common.hpp"
#include "../ui/PauseTheme.hpp"
#include "../ui/PauseMenu.hpp"
#include "../ui/BattleLayout.hpp"
#include "../game/BattleState.hpp"
#include "../game/FaceTextures.hpp"
#include "../game/CardDeck.hpp"
#include "../game/SimpleAuraRenderer.hpp"
#include "../network/MultiplayerManager.hpp"
#include "../network/BattleMessages.hpp"

class Game : public App::Scene
{
public:
	Game(const InitData& init);
	~Game();

	void update() override;

	void draw() const override;

private:
	struct BattleInput
	{
		std::array<bool, 4> attack{};
		bool defend = false;
		bool escape = false;
	};

	class BattleLoop
	{
	public:
		explicit BattleLoop(Game& game) : m_game(game) {}
		virtual ~BattleLoop() = default;
		virtual void onEnter() {}
		virtual void update(const BattleInput& input) = 0;

	protected:
		Game& m_game;
	};

	class PvELoop;
	class PvPHostLoop;
	class PvPClientLoop;

	friend class BattleLoop;
	friend class PvELoop;
	friend class PvPHostLoop;
	friend class PvPClientLoop;

	struct LogEntry
	{
		String message;
		double timestamp = 0.0;
	};

	// 飛翔完了時に適用するダメージ・演出情報
	struct PendingImpact
	{
		bool valid = false;
		int32 hpChange = 0;                 // 削るHP量（0=blocked）
		bool targetIsPlayer = true;         // true=プレイヤーがダメージを受ける
		int32 crazyGain = 0;
		bool crazyTargetIsEnemy = true;     // addCrazy の targetIsEnemy 引数
		net::BattleEventType eventType{};
		int32 primaryValue = 0;
		int32 secondaryValue = 0;
		uint32 flags = 0;
		bool shouldSendStateSync = false;
		bool shouldCheckBattleEnd = false;
	};

	// カード飛翔演出の構造体
	struct CardProjectile
	{
		Vec2 startPos;
		Vec2 targetPos;
		int32 slotIndex = -1;
		String cardName;
		Stopwatch timer{ StartImmediately::No };
		bool active = false;
		bool isBlinking = false;          // 点滅状態
		bool isBlocked = false;           // 防御によって弾かれた攻撃
		bool guardEffectTriggered = false; // 防御エフェクト発動済み
		PendingImpact pendingImpact;
		static constexpr double FlightDuration = 0.5; // 0.5秒で飛ぶ
		static constexpr double BlinkDuration = 0.5;  // 0.5秒間点滅
		static constexpr double TotalDuration = FlightDuration + BlinkDuration; // 合計1.0秒
	};

	// 防御成功エフェクトの構造体
	struct GuardEffect
	{
		Stopwatch timer{ StartImmediately::No };
		bool active = false;
		Vec2 pos;
		static constexpr double Duration = 1.2;
	};
	mutable GuardEffect m_guardEffect;

	// ▼ 追加: 防御ボタン用テクスチャ
	s3d::Texture m_texGuardOn;
	s3d::Texture m_texGuardOff;
	// ▲ 追加ここまで

	// ▼ 追加: ガード演出用テクスチャ（guard.png）
	s3d::Texture m_texGuardEffect;
	void loadGuardEffectTexture();
	// ▲ 追加ここまで

	// プレイヤーアイコン
	s3d::Texture m_texPlayerIcon;

	// 詠唱中画像
	s3d::Texture m_texWriting;

	BattleState m_state;
	FaceTextures m_faces;
	CardDeck m_deck;
	std::unique_ptr<BattleLoop> m_loop;

	s3d::Texture m_texPlayer;
	s3d::Texture m_texPlayerIdleQuantity;
	s3d::Texture m_texPlayerIdleQuality;
	s3d::Texture m_texPlayerIdleCounter;

	s3d::Texture m_texEnemy;
	s3d::Texture m_texEnemyIdleQuantity;
	s3d::Texture m_texEnemyIdleQuality;
	s3d::Texture m_texEnemyIdleCounter;

	s3d::Texture m_texBattleBackground;

	// ▼ 追加: コマンド枠用テクスチャ
	s3d::Texture m_texCommandQuantity;
	s3d::Texture m_texCommandQuality;
	s3d::Texture m_texCommandCounter;
	// ▲ 追加ここまで

	std::shared_ptr<MultiplayerManager> m_multiplayer;
	bool m_isOnlineMode = false;
	bool m_isHost = false;
	bool m_battleEnded = false;

	bool m_paused = false;
	RenderTexture m_sceneRT;
	RenderTexture m_blurInternal;
	RenderTexture m_blurTarget;

	PauseMenu m_pauseMenu;
	Transition m_escapeTr{ 0.3s, 0.15s };

	s3d::Array<LogEntry> m_eventLog;
	static constexpr double LogDisplayDuration = 4.0;
	static constexpr size_t MaxLogEntries = 6;

	CardProjectile m_playerProjectile;
	CardProjectile m_enemyProjectile;

	double m_remoteCostValue = 100.0;
	bool m_remoteDefending = false;
	double m_remoteDefendEndTime = 0.0;

	// ホスト側: クライアントの現在手札を管理（スロット0-3のプールインデックス, 0xFF=未同期）
	std::array<uint8, 4> m_clientActualHand{0xFF, 0xFF, 0xFF, 0xFF};
	std::array<uint8, 4> m_clientVisualHand{0xFF, 0xFF, 0xFF, 0xFF};

	// オーラ描画（差し替え可能）
	std::unique_ptr<IAuraRenderer> m_auraRenderer;

	// プレイヤー攻撃アニメーション（属性別テクスチャ）
	s3d::Texture m_texPlayerAttackQuantity1;
	s3d::Texture m_texPlayerAttackQuantity2;
	s3d::Texture m_texPlayerAttackQuality1;
	s3d::Texture m_texPlayerAttackQuality2;
	s3d::Texture m_texPlayerAttackCounter1;
	s3d::Texture m_texPlayerAttackCounter2;
	Stopwatch m_playerAttackAnimTimer{ StartImmediately::No };
	bool m_playerAttackAnimActive = false;

	// 敵攻撃アニメーション（属性別テクスチャ）
	s3d::Texture m_texEnemyAttackQuantity1;
	s3d::Texture m_texEnemyAttackQuantity2;
	s3d::Texture m_texEnemyAttackQuality1;
	s3d::Texture m_texEnemyAttackQuality2;
	s3d::Texture m_texEnemyAttackCounter1;
	s3d::Texture m_texEnemyAttackCounter2;
	Stopwatch m_enemyAttackAnimTimer{ StartImmediately::No };
	bool m_enemyAttackAnimActive = false;

	static constexpr double AttackAnimFrame1Duration = 0.25; // 1枚目の表示時間（秒）
	static constexpr double AttackAnimFrame2Duration = 0.5; // 2枚目の表示時間（秒）
	static constexpr double AttackAnimTotalDuration = AttackAnimFrame1Duration + AttackAnimFrame2Duration;

	// クレイジーモード突入ズーム演出
	struct CrazyZoomEffect
	{
		Stopwatch timer{ StartImmediately::No };
		bool active = false;
		bool isPlayer = true;
		static constexpr double Duration  = 1.6;  // 演出全体の長さ（秒）
		static constexpr double PeakZoom  = 1.8;  // 最大ズーム倍率
	};
	CrazyZoomEffect m_crazyZoom;

	// 弱点被弾シェイク演出
	struct WeaknessShakeEffect
	{
		Stopwatch timer{ StartImmediately::No };
		bool active = false;
		bool isPlayer = true;
		static constexpr double Duration   = 0.45; // 演出全体の長さ（秒）
		static constexpr double Amplitude  = 12.0; // 最大横ずれ量（px）
		static constexpr double Frequency  = 10.0; // 振動周波数（Hz）
	};
	mutable WeaknessShakeEffect m_weaknessShake;

	void triggerCrazyZoom(bool isPlayer);
	void triggerWeaknessShake(bool isPlayer);
	void setupBattleLoop();

	BattleInput collectBattleInput();
	void updateLogs();
	void finishBattleIfNeeded();
	void concludeBattle(net::BattleEndReason reason, bool hostWon);

	void sendStateSync();
	void applyStateSync(const net::StateSnapshotMessage& msg);
	void broadcastEventToClient(net::BattleEventType type, int32 primaryValue, int32 secondaryValue, uint32 flags);
	void emitLocalEvent(net::BattleEventType type, int32 primaryValue, int32 secondaryValue, uint32 flags);
	String renderBattleEvent(net::BattleEventType type, int32 primaryValue, int32 secondaryValue, uint32 flags, bool localPerspective) const;
	void pushLog(const String& message);

	void handlePlayerAttack(int slotIndex, int32 damage, net::BattleEventType eventType, bool broadcastToClient);
	void handleActionRejected(int reasonCode, net::BattleEventType eventType, bool broadcastToClient);
	void handleDefend(net::BattleEventType eventType, bool broadcastToClient);
	void handleEscape(net::BattleEventType eventType, bool broadcastToClient);
	void handleEnemyAttack(int32 slotIndex, int32 damage, net::BattleEventType eventType, bool broadcastToClient);
	void updateRemoteCost(double deltaTime);
	double remoteAvailableCost() const;
	void consumeRemoteCost(double amount);
	void startRemoteDefend();
	void updateRemoteDefendState();

	void replaceUsedCardIfNeeded();
	void updatePausedUI();
	void performPvEEnemyCounter();

	void startPlayerProjectile(int32 slotIndex, const String& cardName, bool isBlocked = false);
	void startEnemyProjectile(int32 slotIndex, const String& cardName, bool isBlocked = false);
	void updateProjectiles();
	void applyPendingImpact(PendingImpact& impact);
	void triggerGuardEffect(bool isPlayer);
};
