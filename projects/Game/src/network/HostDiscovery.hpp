# pragma once
#include <Siv3D.hpp>

// ホスト情報
struct HostInfo
{
	String hostName;           // ホスト名
	s3d::IPv4Address address;  // IPアドレス
	uint16 port;               // ポート番号
	double lastSeen;           // 最後に検出された時刻
	bool responding;           // 応答があるか
};

// ホスト自動発見クラス（簡易版）
// ローカルネットワークの一般的なIPレンジをスキャン
class HostDiscovery
{
public:
	static constexpr uint16 DEFAULT_GAME_PORT = 12345;
	
	HostDiscovery();
	
	// ホスト検索開始（ローカルネットワークをスキャン）
	void startSearching();
	
	// 停止
	void stop();
	
	// 更新（毎フレーム呼び出し）
	void update();
	
	// 検出されたホスト一覧を取得
	const Array<HostInfo>& getDiscoveredHosts() const { return m_discoveredHosts; }
	
	// 検索中かどうか
	bool isSearching() const { return m_isSearching; }
	
	// 検索進捗（0.0 ~ 1.0）
	double getProgress() const;
	
	// 利用可能なポートを自動検索
	static Optional<uint16> findAvailablePort(uint16 startPort = DEFAULT_GAME_PORT, uint16 range = 100);
	
private:
	bool m_isSearching = false;
	Array<HostInfo> m_discoveredHosts;
	
	// スキャン対象のIPアドレスリスト
	Array<IPv4Address> m_scanTargets;
	size_t m_currentScanIndex = 0;
	
	// タイマー
	double m_lastScanTime = 0.0;
	static constexpr double SCAN_INTERVAL = 0.01;  // 各バッチのスキャン間隔（0.01秒 = 10倍高速化）
	static constexpr size_t BATCH_SIZE = 10;       // 一度にスキャンするホスト数
	
	void generateScanTargets();
	void scanNextHost();
	
	// ローカルIPアドレスを検出
	IPv4Address detectLocalIP();
};

