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
#include "../network/MultiplayerManager.hpp"
#include "../network/BattleMessages.hpp"

class Game : public App::Scene
{
public:
	Game(const InitData& init);

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

	// カード飛翔演出の構造体
	struct CardProjectile
	{
		Vec2 startPos;
		Vec2 targetPos;
		int32 slotIndex = -1;
		String cardName;
		Stopwatch timer{ StartImmediately::No };
		bool active = false;
		bool isBlinking = false;  // 点滅状態
		static constexpr double FlightDuration = 0.5; // 0.5秒で飛ぶ
		static constexpr double BlinkDuration = 0.5;  // 0.5秒間点滅
		static constexpr double TotalDuration = FlightDuration + BlinkDuration; // 合計1.0秒
	};

	BattleState m_state;
	FaceTextures m_faces;
	CardDeck m_deck;
	std::unique_ptr<BattleLoop> m_loop;

	s3d::Texture m_texPlayer;
	s3d::Texture m_texEnemy;

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

	void startPlayerProjectile(int32 slotIndex, const String& cardName);
	void startEnemyProjectile(int32 slotIndex, const String& cardName);
	void updateProjectiles();
};
