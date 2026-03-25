#include "Game_Impl.hpp"
#include "../tools/AudioManager.hpp"

// ネットワークログファイルの実体定義
std::ofstream g_networkLog;

// =============================================================================
//  コンストラクタ・デストラクタ・ループ初期化
// =============================================================================

Game::Game(const InitData& init)
	: IScene{ init }
{
	// --- ログファイルを開く ---
	if (!g_networkLog.is_open()) {
		g_networkLog.open("network_log.txt", std::ios::app);
	}
	m_faces.load();
	m_deck.loadAll();
	if (m_deck.hasCards())
	{
		m_deck.refillRandom(4);
	}

	m_texPlayer = s3d::Texture{ U"assets/ui/characters/player/idle/player.png", s3d::TextureDesc::Unmipped };
	m_texPlayerIdleQuantity = s3d::Texture{ U"assets/ui/characters/player/idle/player_quantity.png", s3d::TextureDesc::Unmipped };
	m_texPlayerIdleQuality  = s3d::Texture{ U"assets/ui/characters/player/idle/player_quality.png",  s3d::TextureDesc::Unmipped };
	m_texPlayerIdleCounter  = s3d::Texture{ U"assets/ui/characters/player/idle/player_counter.png",  s3d::TextureDesc::Unmipped };

	m_texEnemy = s3d::Texture{ U"assets/ui/characters/enemy/idle/enemy.png", s3d::TextureDesc::Unmipped };
	m_texEnemyIdleQuantity = s3d::Texture{ U"assets/ui/characters/enemy/idle/enemy_quantity.png", s3d::TextureDesc::Unmipped };
	m_texEnemyIdleQuality  = s3d::Texture{ U"assets/ui/characters/enemy/idle/enemy_quality.png",  s3d::TextureDesc::Unmipped };
	m_texEnemyIdleCounter  = s3d::Texture{ U"assets/ui/characters/enemy/idle/enemy_counter.png",  s3d::TextureDesc::Unmipped };

	// 攻撃アニメーション用テクスチャ（属性別）
	m_texPlayerAttackQuantity1 = s3d::Texture{ U"assets/ui/characters/player/attack/quantity/player_quantity_1.png", s3d::TextureDesc::Unmipped };
	m_texPlayerAttackQuantity2 = s3d::Texture{ U"assets/ui/characters/player/attack/quantity/player_quantity_2.png", s3d::TextureDesc::Unmipped };
	m_texPlayerAttackQuality1 = s3d::Texture{ U"assets/ui/characters/player/attack/quality/player_quality_1.png", s3d::TextureDesc::Unmipped };
	m_texPlayerAttackQuality2 = s3d::Texture{ U"assets/ui/characters/player/attack/quality/player_quality_2.png", s3d::TextureDesc::Unmipped };
	m_texPlayerAttackCounter1 = s3d::Texture{ U"assets/ui/characters/player/attack/counter/player_counter_1.png", s3d::TextureDesc::Unmipped };
	m_texPlayerAttackCounter2 = s3d::Texture{ U"assets/ui/characters/player/attack/counter/player_counter_2.png", s3d::TextureDesc::Unmipped };

	// 敵攻撃アニメーション用テクスチャ（属性別）
	m_texEnemyAttackQuantity1 = s3d::Texture{ U"assets/ui/characters/enemy/attack/quantity/enemy_quantity_1.png", s3d::TextureDesc::Unmipped };
	m_texEnemyAttackQuantity2 = s3d::Texture{ U"assets/ui/characters/enemy/attack/quantity/enemy_quantity_2.png", s3d::TextureDesc::Unmipped };
	m_texEnemyAttackQuality1 = s3d::Texture{ U"assets/ui/characters/enemy/attack/quality/enemy_quality_1.png", s3d::TextureDesc::Unmipped };
	m_texEnemyAttackQuality2 = s3d::Texture{ U"assets/ui/characters/enemy/attack/quality/enemy_quality_2.png", s3d::TextureDesc::Unmipped };
	m_texEnemyAttackCounter1 = s3d::Texture{ U"assets/ui/characters/enemy/attack/counter/enemy_counter_1.png", s3d::TextureDesc::Unmipped };
	m_texEnemyAttackCounter2 = s3d::Texture{ U"assets/ui/characters/enemy/attack/counter/enemy_counter_2.png", s3d::TextureDesc::Unmipped };

	m_texBattleBackground = s3d::Texture{ U"assets/ui/background/background_battle.png",
										  s3d::TextureDesc::Unmipped };

	m_texCommandQuantity = s3d::Texture{ U"assets/ui/command/command_quantity.png" };
	m_texCommandQuality  = s3d::Texture{ U"assets/ui/command/command_quality.png" };
	m_texCommandCounter  = s3d::Texture{ U"assets/ui/command/command_counter.png" };

	m_texGuardOn  = s3d::Texture{ U"assets/ui/command/guard_on.png" };
	m_texGuardOff = s3d::Texture{ U"assets/ui/command/guard_off.png" };

	m_texPlayerIcon = s3d::Texture{ U"assets/ui/characters/player/player_icon.png" };
	m_texWriting    = s3d::Texture{ U"assets/ui/writing.png" };

	if (getData().multiplayer)
	{
		m_multiplayer = getData().multiplayer;
		m_isOnlineMode = true;
		m_isHost = getData().isHost;
	}

	m_remoteCostValue = 100.0;
	m_remoteDefending = false;
	m_remoteDefendEndTime = 0.0;

	m_auraRenderer = std::make_unique<SimpleAuraRenderer>();

	setupBattleLoop();
	if (m_loop)
	{
		m_loop->onEnter();
	}
	AudioManager::instance().startBGM(U"assets/BGM/battle.mp3", 0.7);
}

Game::~Game()
{
	if (g_networkLog.is_open()) {
		g_networkLog.close();
	}
	AudioManager::instance().stopBGM();
}


