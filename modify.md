# PR #17 修正計画書

**作成日**: 2025-10-31  
**対象**: オンライン対戦（PvP）機能の改善

---

## 📋 目次

1. [修正の全体像](#修正の全体像)
2. [Phase 1: UDPブロードキャストによるホスト検出](#phase-1-udpブロードキャストによるホスト検出)
3. [Phase 2: 通信プロトコルの改善](#phase-2-通信プロトコルの改善)
4. [Phase 3: 接続処理の改善](#phase-3-接続処理の改善)
5. [Phase 4: プラットフォーム抽象化](#phase-4-プラットフォーム抽象化)
6. [実装順序](#実装順序)
7. [テスト計画](#テスト計画)

---

## 修正の全体像

### 現在の問題点
1. **ネットワークスキャン**: 隣接20サブネット（±10）をTCPスキャン → LAN管理者に怒られる可能性
2. **プロトコル**: テキストメッセージをそのまま送信、上限サイズなし
3. **接続処理**: タイムアウトなし、フラグリセット不足
4. **プラットフォーム**: winsock直接使用（Windows専用）

### 修正方針
- **UDP broadcast**による軽量なホスト検出への変更
- **構造化メッセージ**によるプロトコル改善
- **接続タイムアウト・エラーハンドリング**の強化
- **クロスプラットフォーム対応**（段階的）

---

## Phase 1: UDPブロードキャストによるホスト検出

### 1.1 調査結果

#### 参考実装
- **umundo/BroadcastDiscovery**: UDP broadcast基本構造（C++）
- **itisyang/modules**: シンプルなQt実装（255.255.255.255使用）
- **標準的な実装パターン**:
  ```
  1. ホスト: UDPソケット作成 → ブロードキャストアドレスにbind → 定期的にbeacon送信
  2. クライアント: UDPソケット作成 → 255.255.255.255へディスカバリ送信 → 応答待機
  3. ホスト応答: 自分のIP、ポート、ゲーム名などを返信
  ```

#### Siv3Dでの実装方法
Siv3D 0.6.16には以下のネットワーク機能がある:
- `TCPServer` / `TCPClient` (実装済み)
- **問題**: Siv3DにはUDPソケットクラスが存在しない
- **解決策**: 
  - Option A: Siv3D標準機能のみで実装（制限あり）
  - Option B: プラットフォームネイティブAPI使用（winsock/BSD socket）
  - **推奨**: Option B（UDPは軽量なので直接実装が適切）

### 1.2 新規ファイル構成

```
projects/Game/src/network/
├── UDPDiscovery.hpp         (新規)
├── UDPDiscovery.cpp         (新規)
├── NetworkPlatform.hpp      (新規・プラットフォーム抽象化)
├── NetworkPlatform_Win.cpp  (新規・Windows実装)
├── NetworkPlatform_Unix.cpp (新規・Unix/macOS実装)
├── HostDiscovery.hpp        (変更→廃止予定)
├── HostDiscovery.cpp        (変更→廃止予定)
└── ...
```

### 1.3 UDPDiscoveryの設計

#### マッチング方式の変更 🆕
**合言葉（パスワード）方式**を採用：
1. **ホスト側**: 合言葉を設定して部屋を立てる（UDPで待機）
2. **ゲスト側**: 合言葉を入力してブロードキャスト送信（一定間隔で繰り返し）
3. **ハンドシェイク**: ホストが合言葉をチェックし、一致したらレスポンス送信
4. **ゲーム演出**: ホストがSNS投稿 → ゲストがそれに噛みつく（という設定）

**メリット**:
- 意図しない相手との接続を防止
- 友達同士で確実にマッチング可能
- LAN外の相手を排除（セキュリティ向上）

#### クラス構造
```cpp
// UDPDiscovery.hpp
class UDPDiscovery
{
public:
    static constexpr uint16 DISCOVERY_PORT = 12344;  // ゲームポート-1
    static constexpr uint16 GAME_PORT = 12345;
    
    enum class Role { None, Host, Client };
    
    UDPDiscovery();
    ~UDPDiscovery();
    
    // ホスト機能（合言葉で部屋を立てる）
    bool startAdvertising(const String& passphrase, const String& gameName, uint16 gamePort = GAME_PORT);
    void stopAdvertising();
    
    // クライアント機能（合言葉でブロードキャスト）
    bool startSearching(const String& passphrase);
    void stopSearching();
    const Array<HostInfo>& getDiscoveredHosts() const;
    
    // 共通
    void update();  // 毎フレーム呼び出し
    Role getRole() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
```

#### ディスカバリメッセージフォーマット（合言葉対応版）
```cpp
// ディスカバリリクエスト (Client → Broadcast)
// ゲスト側が合言葉付きで一定間隔（2秒ごと）に送信
struct DiscoveryRequest
{
    uint32 magic = 0x52425458;  // "RBTX" (Re-ButtlerX)
    uint8 version = 1;
    uint8 messageType = 0x01;   // REQUEST
    uint16 reserved = 0;
    char passphrase[64];        // 合言葉（UTF-8、null終端）🆕
};

// ディスカバリレスポンス (Host → Client)
// ホスト側が合言葉一致時のみ送信（ハンドシェイク）
struct DiscoveryResponse
{
    uint32 magic = 0x52425458;  // "RBTX"
    uint8 version = 1;
    uint8 messageType = 0x02;   // RESPONSE
    uint16 gamePort;            // TCPゲームポート
    char gameName[32];          // ゲーム名（UTF-8、null終端）
    char hostName[32];          // ホスト名（UTF-8、null終端）
    uint32 timestamp;           // Unix timestamp
    char passphraseHash[32];    // 合言葉の確認用ハッシュ（SHA-256前半16バイト）🆕
};
```

**セキュリティ考慮**:
- 平文送信のリスク: LAN内限定なので許容範囲内
- 将来的な改善: SHA-256ハッシュ化、チャレンジ-レスポンス方式

### 1.4 実装詳細

#### ホスト側（合言葉で部屋を立てる）
```cpp
// 1. UDPソケット作成 (ポート12344でbind)
// 2. SO_REUSEADDR設定
// 3. リクエストを受信 → 合言葉チェック → 一致したらレスポンス送信 🆕

void UDPDiscovery::update()  // ホスト
{
    if (m_role != Role::Host) return;
    
    // 受信データ確認
    while (hasIncomingData())
    {
        auto [data, senderAddr] = receiveFrom();
        
        if (isValidRequest(data))
        {
            DiscoveryRequest* req = reinterpret_cast<DiscoveryRequest*>(data.data());
            
            // 🆕 合言葉チェック（演出: SNS投稿に噛みついた！）
            if (String(req->passphrase) == m_passphrase)
            {
                Console << U"[Host] ゲストが噛みついてきた！ 合言葉一致: " << req->passphrase;
                
                // ハンドシェイク: レスポンス送信
                sendResponseTo(senderAddr);
            }
            else
            {
                Console << U"[Host] 合言葉不一致。無視します。";
            }
        }
    }
}
```

**ゲーム内演出イメージ**:
- ホスト: 「SNSに投稿した！」（合言葉設定完了）
- ゲスト: 「噛みつく！」ボタン → ブロードキャスト送信
- マッチング成功: 「噛みついた相手を発見！」

#### クライアント側（合言葉でブロードキャスト）
```cpp
// 1. UDPソケット作成
// 2. SO_BROADCAST設定
// 3. 255.255.255.255:12344 に合言葉付きリクエストを一定間隔で送信 🆕
// 4. 応答を待機（タイムアウト30秒）

void UDPDiscovery::update()  // クライアント
{
    if (m_role != Role::Client) return;
    
    // 🆕 一定間隔で合言葉付きブロードキャスト（2秒ごと）
    // ホストがオフラインから復帰した場合も自動検出
    if (Scene::Time() - m_lastBroadcastTime > 2.0)
    {
        Console << U"[Client] 合言葉「" << m_passphrase << U"」で噛みつき中...";
        sendDiscoveryRequest(m_passphrase);  // 🆕 合言葉を含める
        m_lastBroadcastTime = Scene::Time();
    }
    
    // 応答を受信（ホストからのハンドシェイク）
    while (hasIncomingData())
    {
        auto [data, senderAddr] = receiveFrom();
        
        if (isValidResponse(data))
        {
            Console << U"[Client] マッチング成功！ホストを発見: " << senderAddr.toString();
            addDiscoveredHost(data, senderAddr);
        }
    }
    
    // 🆕 タイムアウト処理（30秒経過しても見つからない）
    if (Scene::Time() - m_searchStartTime > 30.0)
    {
        Console << U"[Client] タイムアウト: ホストが見つかりませんでした";
        stopSearching();
    }
}
```

**検索タイムアウト**: 30秒（ブロードキャスト15回送信）
**再試行間隔**: 2秒（ホストの一時的なネットワーク切断にも対応）

### 1.5 変更が必要なファイル

#### `HostDiscovery.hpp/cpp`
- **オプション1**: 完全に`UDPDiscovery`に置き換え
- **オプション2**: `HostDiscovery`のバックエンドを`UDPDiscovery`に変更（互換性維持）
- **推奨**: オプション1（クリーンな設計）

#### `Matching.cpp`
```cpp
// 変更前（TCPスキャン）
m_hostDiscovery.startSearching();  // TCPスキャン開始

// 変更後（合言葉マッチング）🆕
// ホスト側
String passphrase = TextInput::Get("合言葉を入力（例: バトル開始）");
m_udpDiscovery.startAdvertising(passphrase, U"Re-ButtlerX", 12345);
Console << U"[演出] SNSに投稿しました！ 合言葉: " << passphrase;

// ゲスト側
String passphrase = TextInput::Get("合言葉を入力して噛みつく");
m_udpDiscovery.startSearching(passphrase);   // 🆕 合言葉付きブロードキャスト
Console << U"[演出] 噛みつき中...";
```

**UI追加要素**:
1. ホスト側: 「合言葉入力欄」（例: "今日のバトル", "友達対戦"）
2. ゲスト側: 「合言葉入力欄」 + 「噛みつく！」ボタン
3. マッチング中: 「相手を探しています...（2秒ごとにブロードキャスト中）」
4. 成功時: 「噛みついた相手を発見！接続します...」

---

## Phase 2: 通信プロトコルの改善

### 2.1 現在の問題

#### BattleTextMessage
```cpp
struct BattleTextMessage
{
    MessageType type = MessageType::BattleMessage;
    String message;  // ❌ 可変長、上限なし、そのまま送信
};
```

**問題点**:
- 文字列の長さが不定（メモリ破壊の危険）
- 送信時にサイズ計算が複雑
- ネットワーク帯域の無駄

### 2.2 改善策

#### イベントベースのメッセージシステム

```cpp
// BattleMessages.hpp に追加

// バトルイベントID
enum class BattleEventID : uint8
{
    // プレイヤー行動
    PlayerAttack1       = 0x01,
    PlayerAttack2       = 0x02,
    PlayerDefend        = 0x03,
    PlayerHeal          = 0x04,
    
    // ダメージ・効果
    DamageDealt         = 0x10,
    DamageBlocked       = 0x11,
    HPRestored          = 0x12,
    
    // 状態変化
    StatusCrazyStart    = 0x20,
    StatusCrazyEnd      = 0x21,
    StatusDefendStart   = 0x22,
    StatusDefendEnd     = 0x23,
    
    // ターン・バトル
    TurnStart           = 0x30,
    TurnEnd             = 0x31,
    BattleStart         = 0x40,
    BattleEnd           = 0x41,
};

// 改善後のイベントメッセージ
struct BattleEventMessage
{
    MessageType type = MessageType::BattleMessage;
    BattleEventID eventID;       // イベント種別
    int32 value1 = 0;            // 数値パラメータ1（例: ダメージ量）
    int32 value2 = 0;            // 数値パラメータ2（例: 残りHP）
    uint32 timestamp = 0;        // タイムスタンプ
    uint8 playerID = 0;          // プレイヤーID (0=Host, 1=Client)
    uint8 reserved[3] = {0};     // 将来の拡張用
};
// 固定サイズ: 20 bytes
```

#### メッセージテーブル
```cpp
// Game.cpp または Common.cpp
const std::map<BattleEventID, String> EVENT_MESSAGES_JP = {
    {BattleEventID::PlayerAttack1, U"攻撃!"},
    {BattleEventID::PlayerDefend, U"防御!"},
    {BattleEventID::DamageDealt, U"{0}のダメージ!"},
    {BattleEventID::HPRestored, U"{0}回復!"},
    // ...
};

String formatBattleMessage(BattleEventID eventID, int32 value1, int32 value2)
{
    String template = EVENT_MESSAGES_JP.at(eventID);
    return template.replaced(U"{0}", Format(value1))
                   .replaced(U"{1}", Format(value2));
}
```

### 2.3 プロトコルドキュメントの作成

#### `docs/NETWORK_PROTOCOL.md` (新規作成)

```markdown
# Re-buttlerX ネットワークプロトコル仕様

## バージョン: 1.0

### メッセージフォーマット

すべてのメッセージは以下の構造:
- リトルエンディアン
- 固定サイズ構造体
- パディングなし (#pragma pack(1) または alignas(1))

### メッセージ一覧

| Type | Name | Size | Description |
|------|------|------|-------------|
| 0x01 | PlayerActionMessage | 12 bytes | プレイヤーの行動 |
| 0x11 | GameStateSyncMessage | 56 bytes | ゲーム状態の同期 |
| 0x12 | TurnChangeMessage | 8 bytes | ターン変更 |
| 0x13 | BattleEventMessage | 20 bytes | バトルイベント |
| 0x14 | BattleEndMessage | 12 bytes | バトル終了 |

### GameStateSyncMessage (0x11)

...（詳細な仕様を記述）
```

#### コード内ドキュメント

```cpp
// BattleMessages.hpp の各構造体に詳細コメント追加

/**
 * @brief ゲーム状態同期メッセージ
 * 
 * サーバー権威方式により、ホスト側が定期的に送信する。
 * クライアント側はこのメッセージを受信して状態を上書きする。
 * 
 * @note サイズ: 56 bytes (固定)
 * @note 送信頻度: 行動完了時、ターン終了時
 * @note エンディアン: Little Endian
 */
struct GameStateSyncMessage
{
    MessageType type = MessageType::GameStateSync;  // 1 byte
    
    // ホスト側の状態 (16 bytes)
    int32 hostHP;               // ホストの現在HP
    double hostCost;            // ホストのコスト
    bool hostDefending;         // ホストが防御中か
    double hostDefendTime;      // 防御開始時刻
    int32 hostCrazy;            // ホストのCrazy値
    
    // クライアント側の状態 (16 bytes)
    int32 clientHP;
    double clientCost;
    bool clientDefending;
    double clientDefendTime;
    int32 clientCrazy;
    
    // バトル状態 (5 bytes)
    bool isHostTurn;            // ホストのターンか
    uint32 turnNumber;          // ターン番号
    
    uint8 reserved[18];         // 将来の拡張用（合計56bytes）
};
```

---

## Phase 3: 接続処理の改善

### 3.1 接続タイムアウトの実装

#### 現在の問題
```cpp
// MultiplayerManager.cpp:19
bool MultiplayerManager::connect(const IPv4Address& address, uint16 port)
{
    if (m_client.connect(address, port))  // ❌ タイムアウトなし
    {
        // 接続成功後、500msまで待機
        for (int i = 0; i < 10; ++i) { ... }
    }
    return false;  // 失敗
}
```

#### 改善版
```cpp
bool MultiplayerManager::connect(const IPv4Address& address, uint16 port, double timeoutSeconds = 5.0)
{
    Console << U"[MultiplayerManager] 接続試行 " << address.str() << U":" << port;
    
    const double startTime = Scene::Time();
    
    if (m_client.connect(address, port))
    {
        m_role = Role::Client;
        Console << U"[MultiplayerManager] TCPClient.connect()成功 - 接続確立を待機中...";
        
        // タイムアウト付き接続確立待機
        while (Scene::Time() - startTime < timeoutSeconds)
        {
            System::Sleep(50ms);
            
            if (m_client.isConnected())
            {
                const double elapsed = Scene::Time() - startTime;
                Console << U"[MultiplayerManager] 接続確立！ (" << (int)(elapsed * 1000) << U"ms)";
                m_clientConnectSucceeded = true;
                m_connectionTimestamp = Scene::Time();
                return true;
            }
        }
        
        // タイムアウト
        Console << U"[MultiplayerManager] 接続タイムアウト (" << timeoutSeconds << U"秒)";
        m_client.disconnect();
        m_clientConnectSucceeded = false;  // ✅ フラグリセット
        m_role = Role::None;
        return false;
    }
    
    Console << U"[MultiplayerManager] 接続失敗";
    m_clientConnectSucceeded = false;  // ✅ フラグリセット
    return false;
}
```

### 3.2 接続状態の監視

```cpp
// MultiplayerManager.hpp に追加
class MultiplayerManager
{
private:
    double m_connectionTimestamp = 0.0;
    double m_lastHeartbeatTime = 0.0;
    static constexpr double HEARTBEAT_INTERVAL = 5.0;  // 5秒ごと
    static constexpr double CONNECTION_TIMEOUT = 15.0;  // 15秒無応答で切断
    
public:
    void update();
    bool isConnectionAlive() const;
};

// MultiplayerManager.cpp
void MultiplayerManager::update()
{
    if (!isConnected())
        return;
    
    // ハートビート送信（定期的にpingを送る）
    if (Scene::Time() - m_lastHeartbeatTime > HEARTBEAT_INTERVAL)
    {
        sendHeartbeat();
        m_lastHeartbeatTime = Scene::Time();
    }
    
    // 接続タイムアウトチェック
    if (m_role == Role::Client)
    {
        if (!m_client.isConnected())
        {
            Console << U"[MultiplayerManager] 接続が切断されました";
            disconnect();
            return;
        }
    }
    else if (m_role == Role::Host)
    {
        if (m_sessionID && !m_server.hasSession(*m_sessionID))
        {
            Console << U"[MultiplayerManager] クライアントが切断されました";
            disconnect();
            return;
        }
    }
    
    processIncomingData();
}
```

### 3.3 ポート固定化

```cpp
// Common.hpp
namespace NetworkConfig
{
    constexpr uint16 UDP_DISCOVERY_PORT = 12344;  // UDP discovery
    constexpr uint16 TCP_GAME_PORT = 12345;       // TCP game
    constexpr uint16 PORT_RANGE = 0;              // 自動検出なし（固定）
}

// HostDiscovery.cpp の複雑なポート検出ロジックを削除
// → 固定ポート使用に簡略化
```

---

## Phase 4: プラットフォーム抽象化

### 4.1 ネットワークプラットフォーム抽象化レイヤー

#### `NetworkPlatform.hpp` (新規)
```cpp
#pragma once
#include <Siv3D.hpp>

namespace NetworkPlatform
{
    // UDP ソケットハンドル（プラットフォーム依存）
    #ifdef _WIN32
        using SocketHandle = SOCKET;
    #else
        using SocketHandle = int;
    #endif
    
    constexpr SocketHandle INVALID_SOCKET_HANDLE = 
        #ifdef _WIN32
            INVALID_SOCKET;
        #else
            -1;
        #endif
    
    // 初期化・終了
    bool InitializeNetworking();
    void ShutdownNetworking();
    
    // UDPソケット作成
    SocketHandle CreateUDPSocket();
    bool CloseSocket(SocketHandle socket);
    
    // ソケットオプション
    bool SetBroadcast(SocketHandle socket, bool enable);
    bool SetReuseAddress(SocketHandle socket, bool enable);
    bool SetNonBlocking(SocketHandle socket, bool enable);
    
    // バインド
    bool BindSocket(SocketHandle socket, uint16 port);
    
    // 送受信
    int32 SendTo(SocketHandle socket, const void* data, size_t size,
                 const IPv4Address& addr, uint16 port);
    int32 ReceiveFrom(SocketHandle socket, void* buffer, size_t bufferSize,
                      IPv4Address& senderAddr, uint16& senderPort);
    
    // ローカルIP取得
    Array<IPv4Address> GetLocalIPAddresses();
    
    // エラー処理
    int32 GetLastError();
    String GetErrorString(int32 errorCode);
}
```

#### `NetworkPlatform_Win.cpp` (新規)
```cpp
#ifdef _WIN32

#include "NetworkPlatform.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

namespace NetworkPlatform
{
    bool InitializeNetworking()
    {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }
    
    void ShutdownNetworking()
    {
        WSACleanup();
    }
    
    SocketHandle CreateUDPSocket()
    {
        return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    
    // ... 他の関数実装
}

#endif  // _WIN32
```

#### `NetworkPlatform_Unix.cpp` (新規)
```cpp
#if defined(__APPLE__) || defined(__linux__)

#include "NetworkPlatform.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

namespace NetworkPlatform
{
    bool InitializeNetworking()
    {
        return true;  // Unix系はWSAStartup不要
    }
    
    void ShutdownNetworking()
    {
        // 何もしない
    }
    
    SocketHandle CreateUDPSocket()
    {
        return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    
    // ... 他の関数実装
}

#endif  // Unix
```

### 4.2 Matching.cpp の修正

#### 現在の問題
```cpp
// Matching.cpp:4
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
```

#### 修正後
```cpp
// Matching.cpp
#include "network/NetworkPlatform.hpp"

// ローカルIP取得
Array<IPv4Address> localIPs = NetworkPlatform::GetLocalIPAddresses();
if (!localIPs.isEmpty())
{
    m_localIP = localIPs[0].str();
}
```

---

## 実装順序

### Step 1: 最小限の修正（即座に対応可能）
**優先度: 🔴 高**  
**所要時間: 2-3時間**

1. ✅ 接続失敗時のフラグリセット（`m_clientConnectSucceeded = false`）
2. ✅ 接続タイムアウトの実装
3. ✅ ポート固定化（`DEFAULT_GAME_PORT = 12345` 固定）
4. ✅ プロトコルドキュメントの作成（`docs/NETWORK_PROTOCOL.md`）

### Step 2: UDPブロードキャスト実装
**優先度: 🔴 高**  
**所要時間: 1-2日**

1. ✅ `NetworkPlatform.hpp/cpp` の実装（Windows版のみ）
2. ✅ `UDPDiscovery.hpp/cpp` の実装
3. ✅ `Matching.cpp` での統合
4. ✅ テスト（同一LAN内での動作確認）
5. ✅ `HostDiscovery.cpp` の段階的廃止

### Step 3: プロトコル改善
**優先度: 🟡 中**  
**所要時間: 1日**

1. ✅ `BattleEventMessage` の実装
2. ✅ `BattleTextMessage` から `BattleEventMessage` への移行
3. ✅ メッセージテーブルの作成
4. ✅ 既存コードの書き換え（`Game.cpp`, `MultiplayerManager.cpp`）

### Step 4: クロスプラットフォーム対応
**優先度: 🔵 低**  
**所要時間: 1-2日**

1. ✅ `NetworkPlatform_Unix.cpp` の実装
2. ✅ macOS/Linux でのテスト
3. ✅ CMakeLists.txt の調整（必要に応じて）

### Step 5: 高度な機能
**優先度: 🔵 低（将来的な拡張）**

1. ⏳ セッション管理の強化（ハートビート、再接続）
2. ⏳ 暗号化（TLS/SSL）
3. ⏳ NAT越え（UPnP、STUN/TURN）

---

## テスト計画

### Test Case 1: UDP Discovery（合言葉マッチング）🆕
**環境**: 同一サブネット内の2台のPC

1. ホスト側: ゲーム起動 → 「ホストとして開始」→ 合言葉「テスト対戦」を設定
2. クライアント側: ゲーム起動 → 「ゲストとして参加」→ 合言葉「テスト対戦」を入力 → 「噛みつく！」
3. **期待結果**: 
   - クライアント側が2秒ごとにブロードキャスト送信
   - ホスト側が合言葉一致を検出 → レスポンス送信
   - クライアント側にホスト情報が表示される（「噛みついた相手を発見！」）
   - IPアドレス、ポート、ゲーム名が正しい
   - 自動的にTCP接続開始

### Test Case 1-2: 合言葉不一致 🆕
**環境**: 同一サブネット内の2台のPC

1. ホスト側: 合言葉「正解ワード」を設定
2. クライアント側: 合言葉「間違ったワード」を入力 → 「噛みつく！」
3. **期待結果**:
   - ホスト側がリクエストを受信するが無視（合言葉不一致）
   - クライアント側は30秒後にタイムアウト
   - エラーメッセージ「ホストが見つかりませんでした」表示

### Test Case 2: 接続タイムアウト
**環境**: クライアント側で無効なIPを入力

1. クライアント側: 存在しないIP（例: 192.168.1.254）に接続試行
2. **期待結果**:
   - 5秒後にタイムアウト
   - エラーメッセージ表示
   - Matching画面に戻る

### Test Case 3: 接続切断検知
**環境**: ゲーム中にネットワークケーブルを抜く

1. ホスト・クライアント接続
2. バトル開始
3. 片方のネットワークを切断
4. **期待結果**:
   - 15秒以内に切断検知
   - エラーメッセージ表示
   - Title画面またはMatching画面に戻る

### Test Case 4: イベントメッセージ
**環境**: 正常な対戦

1. バトル中にカード使用
2. **期待結果**:
   - 日本語メッセージが正しく表示される
   - ダメージ値が正確
   - 遅延が少ない（<100ms）

---

## リスク管理

### リスク 1: UDPがファイアウォールでブロックされる
**発生確率**: 中  
**影響度**: 高  
**対策**: 
- 手動IP入力機能は残す（フォールバック）
- ファイアウォール設定手順をドキュメント化

### リスク 2: Siv3D TCPClient のバグが続く
**発生確率**: 低  
**影響度**: 中  
**対策**:
- Siv3Dのバージョンアップを待つ
- または、TCPもNetworkPlatformで抽象化

### リスク 3: クロスプラットフォーム実装の遅延
**発生確率**: 中  
**影響度**: 低  
**対策**:
- Windows版を優先実装
- macOS/Linux版は後回し（開発者が少ない場合）

---

## 完了基準

### Phase 1完了
- [ ] UDPDiscovery実装完了
- [ ] 同一LAN内でホスト検出成功
- [ ] 既存のTCPスキャンコード削除
- [ ] HostDiscovery.cpp/hpp削除

### Phase 2完了
- [ ] BattleEventMessage実装
- [ ] 全メッセージが構造化
- [ ] NETWORK_PROTOCOL.md作成
- [ ] コード内ドキュメント追加

### Phase 3完了
- [ ] 接続タイムアウト実装
- [ ] 切断検知実装
- [ ] フラグリセット確認
- [ ] ポート固定化

### Phase 4完了
- [ ] NetworkPlatform抽象化
- [ ] Windows/macOS/Linux対応
- [ ] winsock直接使用の削除

---

## 参考資料

### 実装例
- [umundo/BroadcastDiscovery.cpp](https://github.com/tklab-tud/umundo/blob/master/src/umundo/discovery/BroadcastDiscovery.cpp)
- [itisyang/modules - UDP discovery](https://github.com/itisyang/modules/tree/master/device_discovery)

### UDP Broadcast基本
- [ZeroMQ Beacon実装](https://github.com/zeromq/czmq/blob/master/src/zbeacon.c)
- [RFC 919 - Broadcasting Internet Datagrams](https://www.rfc-editor.org/rfc/rfc919.html)

### Siv3D関連
- [Siv3D Documentation - Network](https://siv3d.github.io/ja-jp/)
- [Siv3D GitHub Issues](https://github.com/Siv3D/OpenSiv3D/issues)

---

## 更新履歴

- **2025-10-31**: 初版作成
- **2025-10-31 (更新)**: 🆕 合言葉（パスワード）マッチング方式を追加
  - ホスト側: 合言葉設定 → UDPで待機
  - ゲスト側: 合言葉入力 → 一定間隔でブロードキャスト
  - ハンドシェイク: 合言葉一致時のみレスポンス
  - ゲーム演出: 「SNS投稿」→「噛みつく」コンセプト
- **[次回更新日]**: Phase 1実装結果を反映予定

