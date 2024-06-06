#include "SocketOps.h"
#include "../Scheduler/Log.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
//#include <sys/uio.h>
//#include <sys/ioctl.h>
//#include <netinet/in.h>


int sockets::tcpSocket()
{
	return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

int sockets::udpSocket()
{
	return ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
}

bool sockets::bind(int fd, std::string ip, uint16_t port)
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	addr.sin_port = htons(port);

	if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		LOGE("bind error");
		return false;
	}
	return true;
}

bool sockets::listen(int fd, int num)
{
	if (::listen(fd, num) < 0)
	{
		LOGE("listen error");
		return false;
	}
	return true;
}

void sockets::setNoneblock_CloseOnExe(int fd)
{
	int flag = ::fcntl(fd, F_GETFL, 0);
	flag |= O_NONBLOCK;
	::fcntl(fd, F_SETFL, flag);

	flag = ::fcntl(fd, F_GETFL, 0);
	flag |= FD_CLOEXEC;
	::fcntl(fd, F_SETFL, flag);
	//disapart write to avoid adderror( |
}

void sockets::setIgonrePipe(int fd)
{
	int option = 1;
	setsockopt(fd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
}

int sockets::accept(int fd)
{
	sockaddr_in addr;
	//sockaddr addr;
	socklen_t addr_len = sizeof(sockaddr_in);
	int connfd = ::accept(fd, (sockaddr*)&addr, &addr_len);
	if (connfd < 0)
	{
		LOGE("accept error");
		return 0;
	}
	setNoneblock_CloseOnExe(connfd);
	setIgonrePipe(connfd);
	return connfd;
	//sigpipe generate from writing to colsed pipe or socket, lead to end process
}


void sockets::close(int fd)
{
	::close(fd);
}

void sockets::setAddreuse(int fd, int on)
{
	int optval = on ? 1 : 0;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

int sockets::write(int fd, const void* data, int size)
{
	return ::write(fd, data, size);
}

int sockets::sendto(int fd, const void* data, int len, const sockaddr* destaddr)
{
	int sizeaddr = sizeof(sockaddr);
	return ::sendto(fd, (char*)data, len, 0, destaddr, sizeaddr);
}

std::string sockets::getLocalIp()
{
	return "0.0.0.0";
}