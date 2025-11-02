#include "NetworkPlatform.hpp"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace NetworkPlatform
{
	bool InitializeNetworking()
	{
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0)
		{
			//Console << U"[NetworkPlatform] WSAStartup failed: " << result;
			return false;
		}
		//Console << U"[NetworkPlatform] Winsock initialized";
		return true;
	}

	void ShutdownNetworking()
	{
		WSACleanup();
		//Console << U"[NetworkPlatform] Winsock shutdown";
	}

	SocketHandle CreateUDPSocket()
	{
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock == INVALID_SOCKET)
		{
			//Console << U"[NetworkPlatform] socket() failed: " << WSAGetLastError();
			return INVALID_SOCKET_HANDLE;
		}
		return static_cast<SocketHandle>(sock);
	}

	void CloseSocket(SocketHandle socket)
	{
		if (socket != INVALID_SOCKET_HANDLE)
		{
			closesocket(static_cast<SOCKET>(socket));
		}
	}

	bool SetNonBlocking(SocketHandle socket, bool enable)
	{
		u_long mode = enable ? 1u : 0u;
		int result = ioctlsocket(static_cast<SOCKET>(socket), FIONBIO, &mode);
		if (result != 0)
		{
			//Console << U"[NetworkPlatform] ioctlsocket(FIONBIO) failed: " << WSAGetLastError();
			return false;
		}
		return true;
	}

	bool SetReuseAddress(SocketHandle socket, bool enable)
	{
		int optval = enable ? 1 : 0;
		int result = setsockopt(
			static_cast<SOCKET>(socket),
			SOL_SOCKET,
			SO_REUSEADDR,
			reinterpret_cast<const char*>(&optval),
			sizeof(optval)
		);
		if (result == SOCKET_ERROR)
		{
			//Console << U"[NetworkPlatform] setsockopt(SO_REUSEADDR) failed: " << WSAGetLastError();
			return false;
		}
		return true;
	}

	bool SetBroadcast(SocketHandle socket, bool enable)
	{
		int optval = enable ? 1 : 0;
		int result = setsockopt(
			static_cast<SOCKET>(socket),
			SOL_SOCKET,
			SO_BROADCAST,
			reinterpret_cast<const char*>(&optval),
			sizeof(optval)
		);
		if (result == SOCKET_ERROR)
		{
			//Console << U"[NetworkPlatform] setsockopt(SO_BROADCAST) failed: " << WSAGetLastError();
			return false;
		}
		return true;
	}

	bool BindSocket(SocketHandle socket, uint16 port)
	{
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);

		int result = bind(
			static_cast<SOCKET>(socket),
			reinterpret_cast<const sockaddr*>(&addr),
			sizeof(addr)
		);
		if (result == SOCKET_ERROR)
		{
			//Console << U"[NetworkPlatform] bind() failed: " << WSAGetLastError();
			return false;
		}
		return true;
	}

	int32 SendTo(SocketHandle socket, const void* data, size_t size, const IPv4Address& address, uint16 port)
	{
		sockaddr_in dest{};
		dest.sin_family = AF_INET;
		dest.sin_port = htons(port);

		std::string addrStr = address.str().narrow();
		inet_pton(AF_INET, addrStr.c_str(), &dest.sin_addr);

		int result = sendto(
			static_cast<SOCKET>(socket),
			reinterpret_cast<const char*>(data),
			static_cast<int>(size),
			0,
			reinterpret_cast<const sockaddr*>(&dest),
			sizeof(dest)
		);

		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
			{
				//Console << U"[NetworkPlatform] sendto() failed: " << error;
			}
			return -1;
		}

		return result;
	}

	RecvFromResult RecvFrom(SocketHandle socket, void* buffer, size_t bufferSize)
	{
		sockaddr_in sender{};
		int senderSize = sizeof(sender);

		int result = recvfrom(
			static_cast<SOCKET>(socket),
			reinterpret_cast<char*>(buffer),
			static_cast<int>(bufferSize),
			0,
			reinterpret_cast<sockaddr*>(&sender),
			&senderSize
		);

		RecvFromResult recvResult{};
		recvResult.bytesReceived = result;

		if (result == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
			{
				//Console << U"[NetworkPlatform] recvfrom() failed: " << error;
			}
			recvResult.bytesReceived = -1;
			return recvResult;
		}

		if (result > 0)
		{
			recvResult.senderAddress = IPv4Address{
				sender.sin_addr.S_un.S_un_b.s_b1,
				sender.sin_addr.S_un.S_un_b.s_b2,
				sender.sin_addr.S_un.S_un_b.s_b3,
				sender.sin_addr.S_un.S_un_b.s_b4
			};
			recvResult.senderPort = ntohs(sender.sin_port);
		}

		return recvResult;
	}

	int32 GetLastError()
	{
		return WSAGetLastError();
	}

	String GetErrorString(int32 errorCode)
	{
		return Format(U"Error code: ", errorCode);
	}

	Array<IPv4Address> GetLocalIPAddresses()
	{
		Array<IPv4Address> result;

		ULONG bufferSize = 15000;
		Array<uint8> buffer(bufferSize);
		PIP_ADAPTER_ADDRESSES adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

		ULONG ret = GetAdaptersAddresses(
			AF_INET,
			GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
			nullptr,
			adapterAddresses,
			&bufferSize
		);

		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			buffer.resize(bufferSize);
			adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

			ret = GetAdaptersAddresses(
				AF_INET,
				GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
				nullptr,
				adapterAddresses,
				&bufferSize
			);
		}

		if (ret != NO_ERROR)
		{
			//Console << U"[NetworkPlatform] GetAdaptersAddresses failed: " << ret;
			return result;
		}

		for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter != nullptr; adapter = adapter->Next)
		{
			if (adapter->OperStatus != IfOperStatusUp)
			{
				continue;
			}

			for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next)
			{
				if (unicast->Address.lpSockaddr->sa_family == AF_INET)
				{
					sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
					IPv4Address ip{
						addr->sin_addr.S_un.S_un_b.s_b1,
						addr->sin_addr.S_un.S_un_b.s_b2,
						addr->sin_addr.S_un.S_un_b.s_b3,
						addr->sin_addr.S_un.S_un_b.s_b4
					};

					if (ip != IPv4Address{ 127, 0, 0, 1 })
					{
						result.push_back(ip);
					}
				}
			}
		}

		return result;
	}
}

#endif // _WIN32
