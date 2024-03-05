#pragma once
#include<string>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

namespace sockets
{
	int Create_TcpSocket(); //default unblock
	int Create_UdpSocket();
	bool bind(int fd, std::string ip, uint16_t port);
	bool listen(int fd, int num);
	int accept(int fd);
	int write(int fd, const void* buf, int size); //tcp write
	int sendto(int fd, const void* buf, int len, const sockaddr* destaddr); //udp write
	void close(int fd);
	bool connect(int fd, std::string ip, uint16_t port, int timeout);

	int Set_NoneBlock(int fd);
	int Set_Block(int fd, int write_timeout);
	void Set_ReuseAddr(int fd, int on);
	void Set_ReusePort(int fd);
	void Set_NoneBlock_CloseOnExec(int fd);
	void Ignore_SigPipe_OnSocket(int fd);
	void Set_NoDelay(int fd);
	void Set_KeepAlive(int fd);
	void Set_NoSigpipe(int fd);
	void Set_SendBufSize(int fd, int size);
	void Set_RecvBufSize(int fd, int size);

	std::string Get_PeerIp(int fd);
	int16_t Get_PeerPort(int fd);
	int Get_PeerAddr(int fd, sockaddr_in* addr);
	std::string Get_LocalIp();
}