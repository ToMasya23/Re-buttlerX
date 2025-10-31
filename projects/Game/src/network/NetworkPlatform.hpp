#pragma once
#include <Siv3D.hpp>

// プラットフォーム非依存のネットワーク機能抽象化レイヤー
namespace NetworkPlatform
{
#ifdef _WIN32
	using SocketHandle = uintptr_t;  // SOCKET型
#else
	using SocketHandle = int;
#endif

	constexpr SocketHandle INVALID_SOCKET_HANDLE = static_cast<SocketHandle>(-1);

	// ネットワークライブラリの初期化（Windows: WSAStartup）
	bool InitializeNetworking();

	// ネットワークライブラリの終了（Windows: WSACleanup）
	void ShutdownNetworking();

	// UDPソケットの作成
	SocketHandle CreateUDPSocket();

	// ソケットのクローズ
	void CloseSocket(SocketHandle socket);

	// ソケットを非ブロッキングモードに設定
	bool SetNonBlocking(SocketHandle socket, bool enable);

	// SO_REUSEADDR設定
	bool SetReuseAddress(SocketHandle socket, bool enable);

	// SO_BROADCAST設定（ブロードキャスト送信許可）
	bool SetBroadcast(SocketHandle socket, bool enable);

	// ソケットをアドレスとポートにバインド
	bool BindSocket(SocketHandle socket, uint16 port);

	// データ送信（UDP: sendto）
	int32 SendTo(SocketHandle socket, const void* data, size_t size, const IPv4Address& address, uint16 port);

	// データ受信（UDP: recvfrom）
	struct RecvFromResult
	{
		int32 bytesReceived;
		IPv4Address senderAddress;
		uint16 senderPort;
	};
	RecvFromResult RecvFrom(SocketHandle socket, void* buffer, size_t bufferSize);

	// 最後のエラーコードを取得
	int32 GetLastError();

	// エラーコードを文字列に変換
	String GetErrorString(int32 errorCode);

	// ローカルIPアドレスを取得
	Array<IPv4Address> GetLocalIPAddresses();
}
