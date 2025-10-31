# UDP Discovery実装完了報告

## 実装日時
2025-10-31

## 実装内容

### 新規作成ファイル

1. **NetworkPlatform.hpp** (C:\Users\Yuta_Matsuo\siv3d\test2\Re-buttlerX\projects\Game\src\network\NetworkPlatform.hpp)
   - プラットフォーム非依存のネットワーク抽象化レイヤー
   - UDP ソケット操作の抽象化

2. **NetworkPlatform_Win.cpp** (C:\Users\Yuta_Matsuo\siv3d\test2\Re-buttlerX\projects\Game\src\network\NetworkPlatform_Win.cpp)
   - Windows版の実装（Winsock2使用）
   - ローカルIPアドレス取得機能

3. **UDPDiscovery.hpp** (C:\Users\Yuta_Matsuo\siv3d\test2\Re-buttlerX\projects\Game\src\network\UDPDiscovery.hpp)
   - 合言葉ベースのUDPブロードキャスト検出クラス
   - DiscoveryRequest / DiscoveryResponse 構造体定義

4. **UDPDiscovery.cpp** (C:\Users\Yuta_Matsuo\siv3d\test2\Re-buttlerX\projects\Game\src\network\UDPDiscovery.cpp)
   - ホスト側：合言葉で部屋を立てる
   - ゲスト側：合言葉でブロードキャスト送信（2秒間隔）
   - ハンドシェイク機能

### 修正ファイル

1. **Matching.hpp**
   - UDPDiscovery インクルード追加
   - ViewMode::PassphraseInput 追加
   - m_udpDiscovery メンバー追加
   - m_passphraseInputState メンバー追加

2. **Matching.cpp**
   - NetworkPlatform.hpp インクルード
   - winsock直接使用を削除
   - detectLocalIPForDisplay() を NetworkPlatform::GetLocalIPAddresses() 使用に変更
   - 合言葉入力画面の実装
   - UDP Discovery 統合
   - ゲーム演出：「SNS投稿」→「噛みつく」コンセプト

## 機能概要

### 合言葉マッチング方式

#### ホスト側
1. 「ホストとして開始」ボタンをクリック
2. 合言葉入力画面（例：「今日のバトル」）
3. 「部屋を立てる」ボタン
4. UDPポート12344で待機
5. TCPポート12345でゲーム接続待機
6. 演出：「SNSに投稿しました！」

#### ゲスト側
1. 「ゲストとして参加」ボタンをクリック
2. 合言葉入力画面（ホストと同じ合言葉を入力）
3. 「噛みつく！」ボタン
4. 2秒ごとにUDPブロードキャスト送信（合言葉付き）
5. ホスト発見時に自動TCP接続
6. 演出：「噛みつき中...」→「噛みついた相手を発見！」

### メッセージフォーマット

#### DiscoveryRequest（ゲスト→ブロードキャスト）
```cpp
struct DiscoveryRequest {
    uint32 magic = 0x52425458;  // "RBTX"
    uint8 version = 1;
    uint8 messageType = 0x01;
    uint16 reserved = 0;
    char passphrase[64];        // 合言葉（UTF-8）
};
```

#### DiscoveryResponse（ホスト→ゲスト）
```cpp
struct DiscoveryResponse {
    uint32 magic = 0x52425458;
    uint8 version = 1;
    uint8 messageType = 0x02;
    uint16 gamePort;            // TCPゲームポート
    char gameName[32];          // ゲーム名
    char hostName[32];          // ホスト名
    uint32 timestamp;
    char passphraseHash[32];    // 将来の拡張用
};
```

## ビルド手順

### Visual Studio プロジェクトへのファイル追加

Game.vcxproj に以下のファイルを追加する必要があります：

```xml
<ItemGroup>
  <ClCompile Include="src\network\NetworkPlatform_Win.cpp" />
  <ClCompile Include="src\network\UDPDiscovery.cpp" />
  <!-- 既存のファイル -->
</ItemGroup>

<ItemGroup>
  <ClInclude Include="src\network\NetworkPlatform.hpp" />
  <ClInclude Include="src\network\UDPDiscovery.hpp" />
  <!-- 既存のファイル -->
</ItemGroup>
```

### ビルドコマンド（コマンドプロンプト）

```cmd
cd C:\Users\Yuta_Matsuo\siv3d\test2\Re-buttlerX\projects\Game
MSBuild.exe Game.sln /p:Configuration=Debug /p:Platform=x64 /t:Build /m
```

または Visual Studio で開いて F5 でビルド・実行。

## テスト手順

### Test Case 1: 合言葉マッチング成功

**準備：** 同一LAN内の2台のPC（または同一PC）

**手順：**
1. PC1（ホスト）：ゲーム起動 → 「ホストとして開始」
2. PC1：合言葉「テスト対戦」を入力 → 「部屋を立てる」
3. PC2（ゲスト）：ゲーム起動 → 「ゲストとして参加」
4. PC2：合言葉「テスト対戦」を入力 → 「噛みつく！」
5. 自動的にマッチング → Game画面へ遷移

**期待結果：**
- ゲストが2秒ごとにブロードキャスト
- ホストが合言葉一致を検出
- 自動的にTCP接続確立
- コンソールに「[演出] 噛みついた相手を発見！」表示

### Test Case 2: 合言葉不一致

**手順：**
1. PC1：合言葉「正解ワード」
2. PC2：合言葉「間違ったワード」

**期待結果：**
- ホストは無視（コンソールに「合言葉不一致」）
- ゲストは30秒後にタイムアウト

## 既知の制限事項

1. **ファイアウォール**: UDP 12344ポートが開いている必要がある
2. **LAN限定**: インターネット越えには対応していない
3. **合言葉平文**: セキュリティ強化は将来の拡張（SHA-256ハッシュ化予定）

## 次のステップ（modify.md参照）

- [ ] Step 2完了チェック
- [ ] 実機テスト（2台のPC）
- [ ] ポート固定化確認
- [ ] HostDiscovery.cpp の段階的廃止
- [ ] Step 3: プロトコル改善（BattleEventMessage）

## 参考
- modify.md: 全体の実装計画
- Phase 1完了: UDPブロードキャストによるホスト検出
