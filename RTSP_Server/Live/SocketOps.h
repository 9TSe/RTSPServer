#pragma once
#include <string>
#include <sys/socket.h>

namespace sockets
{
	int tcpSocket();
	int udpSocket();
	bool bind(int fd, std::string ip, uint16_t port);
	bool listen(int fd, int num);
	int accept(int fd);
	void close(int fd);
	int write(int fd, const void* data, int size);
	int sendto(int fd, const void* data, int len, const sockaddr* destaddr);

	void setNoneblock_CloseOnExe(int fd);
	void setIgonrePipe(int fd);
	void setAddreuse(int fd, int on);

	std::string getLocalIp();
}