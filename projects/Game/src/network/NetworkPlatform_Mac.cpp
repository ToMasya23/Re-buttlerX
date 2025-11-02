#include "NetworkPlatform.hpp"

#if defined(__APPLE__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <cerrno>
#include <cstring>
#include <string>

namespace NetworkPlatform
{
	bool InitializeNetworking()
	{
		return true;
	}

	void ShutdownNetworking()
	{
		// BSD sockets require no global shutdown
	}

	SocketHandle CreateUDPSocket()
	{
		int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0)
		{
			//Console << U"[NetworkPlatform] socket() failed: " << errno;
			return INVALID_SOCKET_HANDLE;
		}
		return static_cast<SocketHandle>(sock);
	}

	void CloseSocket(SocketHandle socket)
	{
		if (socket != INVALID_SOCKET_HANDLE)
		{
			::close(static_cast<int>(socket));
		}
	}

	bool SetNonBlocking(SocketHandle socket, bool enable)
	{
		int flags = ::fcntl(static_cast<int>(socket), F_GETFL, 0);
		if (flags < 0)
		{
			//Console << U"[NetworkPlatform] fcntl(F_GETFL) failed: " << errno;
			return false;
		}

		if (enable)
		{
			flags |= O_NONBLOCK;
		}
		else
		{
			flags &= ~O_NONBLOCK;
		}

		if (::fcntl(static_cast<int>(socket), F_SETFL, flags) < 0)
		{
			//Console << U"[NetworkPlatform] fcntl(F_SETFL) failed: " << errno;
			return false;
		}

		return true;
	}

	bool SetReuseAddress(SocketHandle socket, bool enable)
	{
		int optval = enable ? 1 : 0;
		if (::setsockopt(static_cast<int>(socket), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
		{
			//Console << U"[NetworkPlatform] setsockopt(SO_REUSEADDR) failed: " << errno;
			return false;
		}

#ifdef SO_REUSEPORT
		::setsockopt(static_cast<int>(socket), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
#endif

		return true;
	}

	bool SetBroadcast(SocketHandle socket, bool enable)
	{
		int optval = enable ? 1 : 0;
		if (::setsockopt(static_cast<int>(socket), SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0)
		{
			//Console << U"[NetworkPlatform] setsockopt(SO_BROADCAST) failed: " << errno;
			return false;
		}

		return true;
	}

	bool BindSocket(SocketHandle socket, uint16 port)
	{
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);

		if (::bind(static_cast<int>(socket), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
		{
			//Console << U"[NetworkPlatform] bind() failed: " << errno;
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
		if (::inet_pton(AF_INET, addrStr.c_str(), &dest.sin_addr) != 1)
		{
			//Console << U"[NetworkPlatform] inet_pton() failed for " << address.str();
			return -1;
		}

		ssize_t sent = ::sendto(
			static_cast<int>(socket),
			data,
			size,
			0,
			reinterpret_cast<const sockaddr*>(&dest),
			sizeof(dest)
		);

		if (sent < 0)
		{
			int error = errno;
			if (error != EWOULDBLOCK && error != EAGAIN)
			{
				//Console << U"[NetworkPlatform] sendto() failed: " << error;
			}
			return -1;
		}

		return static_cast<int32>(sent);
	}

	RecvFromResult RecvFrom(SocketHandle socket, void* buffer, size_t bufferSize)
	{
		sockaddr_in sender{};
		socklen_t senderSize = sizeof(sender);

		ssize_t received = ::recvfrom(
			static_cast<int>(socket),
			buffer,
			bufferSize,
			0,
			reinterpret_cast<sockaddr*>(&sender),
			&senderSize
		);

		RecvFromResult result{};

		if (received < 0)
		{
			int error = errno;
			if (error != EWOULDBLOCK && error != EAGAIN)
			{
				//Console << U"[NetworkPlatform] recvfrom() failed: " << error;
			}
			result.bytesReceived = -1;
			return result;
		}

		result.bytesReceived = static_cast<int32>(received);
		if (received > 0)
		{
			uint32 addr = ntohl(sender.sin_addr.s_addr);
			result.senderAddress = IPv4Address{
				static_cast<uint8>((addr >> 24) & 0xFF),
				static_cast<uint8>((addr >> 16) & 0xFF),
				static_cast<uint8>((addr >> 8) & 0xFF),
				static_cast<uint8>(addr & 0xFF)
			};
			result.senderPort = ntohs(sender.sin_port);
		}

		return result;
	}

	int32 GetLastError()
	{
		return errno;
	}

	String GetErrorString(int32 errorCode)
	{
		return Unicode::FromUTF8(std::strerror(errorCode));
	}

	Array<IPv4Address> GetLocalIPAddresses()
	{
		Array<IPv4Address> result;
		ifaddrs* ifaddr = nullptr;

		if (::getifaddrs(&ifaddr) != 0)
		{
			//Console << U"[NetworkPlatform] getifaddrs() failed: " << errno;
			return result;
		}

		for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
		{
			if (!ifa->ifa_addr)
			{
				continue;
			}

			if ((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_LOOPBACK) != 0)
			{
				continue;
			}

			if (ifa->ifa_addr->sa_family == AF_INET)
			{
				auto* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
				uint32 addr = ntohl(sa->sin_addr.s_addr);
				IPv4Address ip{
					static_cast<uint8>((addr >> 24) & 0xFF),
					static_cast<uint8>((addr >> 16) & 0xFF),
					static_cast<uint8>((addr >> 8) & 0xFF),
					static_cast<uint8>(addr & 0xFF)
				};

				if (ip != IPv4Address{ 127, 0, 0, 1 })
				{
					result.push_back(ip);
				}
			}
		}

		::freeifaddrs(ifaddr);

		if (result.isEmpty())
		{
			result.push_back(IPv4Address{ 127, 0, 0, 1 });
		}

		return result;
	}
}

#endif // __APPLE__
