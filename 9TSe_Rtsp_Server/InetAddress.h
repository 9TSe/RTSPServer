#pragma once
#include <string>
#include <stdint.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

class Ipv4Address
{
public:
	Ipv4Address();
	Ipv4Address(std::string ip, uint16_t port);
	void Set_Addr(std::string ip, uint16_t port);
	std::string Get_Ip();
	uint16_t Get_Port();
	sockaddr* Get_Addr();

private:
	std::string m_ip;
	uint16_t m_port;
	sockaddr_in m_addr;
};