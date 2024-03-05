#include "SocketsOps.h"
#include <fcntl.h>
#include <sys/types.h>
#include "Log.h"

#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif

int sockets::Create_TcpSocket()
{
#ifndef _WIN32
	int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
#else
	int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	unsigned long ul = 1;
	//second parameter, ul = 0 block, or unblock
	int ret = ioctlsocket(fd, FIONBIO, (unsigned long*)&ul); //unblock

	if (ret == SOCKET_ERROR)
		LOGE("set unblock fail");
#endif
	return fd;
}

int sockets::Create_UdpSocket()
{
#ifndef _WIN32
	int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
#else
	int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	unsigned long ul = 1;
	int ret = ioctlsocket(fd, FIONBIO, (unsigned long*)&ul);
	if (ret == SOCKET_ERROR)
		LOGE("set unblock fail");
#endif
	return fd;
}

bool sockets::bind(int fd, std::string ip, uint16_t port)
{
	/*
	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	server_addr.sin_addr.s_addr = inet_addr("192.168.0.1");
	server_addr.sin_port = htons(port);
	
	int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (::bind(fd, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR)
		return false;
	else
		return true;
	*/

	sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.c_str()); // ->int
	addr.sin_port = htons(port);

	if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		LOGE("::bind error, fd = %d, ip = %s, port = %d", fd, ip.c_str(), port);
		return false;
	}
	return true;
}

bool sockets::listen(int fd, int num)
{
	if (::listen(fd, num) < 0)
	{
		LOGE("::listen error, fd = %d, num = %d", fd, num);
		return false;
	}
	return true;
}

int sockets::accept(int fd)
{
	sockaddr_in addr = { 0 };
	socklen_t addrlen = sizeof(sockaddr_in);

	int connect_fd = ::accept(fd, (sockaddr*)&addr, &addrlen);
	Set_NoneBlock_CloseOnExec(connect_fd);
	Ignore_SigPipe_OnSocket(connect_fd);
	return connect_fd;
}

bool sockets::connect(int fd, std::string ip, uint16_t port, int timeout)
{
	bool isconnected = true;
	if (timeout > 0)
		sockets::Set_NoneBlock(fd);

	sockaddr_in addr = { 0 };
	socklen_t addrlen = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	if (::connect(fd, (sockaddr*)&addr, addrlen) < 0)
	{
		if (timeout > 0)
		{
			isconnected = false;
			fd_set fd_write;
			FD_ZERO(&fd_write);
			FD_SET(fd, &fd_write);

			timeval tv = { timeout / 1000, timeout % 1000 * 1000 };
			select(fd + 1, nullptr, &fd_write, nullptr, &tv);
			if (FD_ISSET(fd, &fd_write))
				isconnected = true;

			sockets::Set_Block(fd, 0);
		}
		else
			isconnected = false;
	}
	return isconnected;
}

void sockets::close(int fd)
{
#ifndef _WIN32
	int ret = ::close(fd);
#else
	int ret = ::closesocket(fd);
#endif
}

int sockets::write(int fd, const void* buf, int size)
{
#ifndef _WIN32
	return ::write(fd, buf, size);
#else
	return ::send(fd, (char*)buf, size, 0);
#endif
}

int sockets::sendto(int fd, const void* buf, int len, const sockaddr* destaddr)
{
	socklen_t addrlen = sizeof(sockaddr);
	return ::sendto(fd, (char*)buf, len, 0, destaddr, addrlen);
}

int sockets::Set_NoneBlock(int fd)
{
#ifndef _WIN32
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	return 0;
#else
	unsigned long ul = 1;
	int ret = ioctlsocket(fd, FIONBIO, (unsigned long*)&ul);
	if (ret == SOCKET_ERROR)
		return -1;
	else
		return 0;
#endif
}

int sockets::Set_Block(int fd, int write_timeout)
{
#ifndef _WIN32
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));

	if (write_timeout > 0)
	{
		timeval tv = { write_timeout / 1000, (write_timeout % 1000) * 1000 };
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
	}
#endif
	return 0;
}

void sockets::Set_ReuseAddr(int fd, int on)
{
	int optval = on ? 1 : 0;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
}

void sockets::Set_ReusePort(int fd)
{
#ifdef SO_REUSEPORT
	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&on, sizeof(on));
#endif
}

void sockets::Set_NoneBlock_CloseOnExec(int fd)
{
#ifndef _WIN32
	int flags = ::fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int ret = ::fcntl(fd, F_SETFL, flags);

	flags = ::fcntl(fd, F_GETFL, 0);
	flags |= FD_CLOEXEC;
	ret = ::fcntl(fd, F_SETFD, flags);
#endif 
}

void sockets::Ignore_SigPipe_OnSocket(int fd)
{
#ifndef _WIN32
	int option = 1;
	setsockopt(fd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
#endif 
}

void sockets::Set_NoDelay(int fd)
{
#ifdef TCP_NODELAY
	int on = 1;
	int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
#endif 
}

void sockets::Set_KeepAlive(int fd)
{
	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
}

void sockets::Set_NoSigpipe(int fd)
{
#ifdef SO_NOSIGPIPE
	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&on, sizeof(on));
#endif
}

void sockets::Set_SendBufSize(int fd, int size)
{
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
}

void sockets::Set_RecvBufSize(int fd, int size)
{
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
}

std::string sockets::Get_PeerIp(int fd)
{
	sockaddr_in addr = { 0 };
	socklen_t addrlen = sizeof(sockaddr_in);
	if (getpeername(fd, (sockaddr*)&addr, &addrlen) == 0)
		return inet_ntoa(addr.sin_addr);
	return "0.0.0.0";
}

int16_t sockets::Get_PeerPort(int fd)
{
	sockaddr_in addr = { 0 };
	socklen_t addrlen = sizeof(sockaddr_in);
	if (getpeername(fd, (sockaddr*)&addr, &addrlen) == 0)
		return ntohs(addr.sin_port);
	return 0;
}

int sockets::Get_PeerAddr(int fd, sockaddr_in* addr)
{
	socklen_t addrlen = sizeof(sockaddr_in);
	return getpeername(fd, (sockaddr*)addr, &addrlen);
}

std::string sockets::Get_LocalIp()
{
	return "0.0.0.0";
}