#pragma once
#include <string>
#include <arpa/inet.h>

class IPV4Address
{
public:
	IPV4Address();
	IPV4Address(std::string ip, uint16_t port);
	void Set_Address(std::string ip, uint16_t port);

	std::string getIp() { return m_ip; }
	uint16_t getPort() { return m_port; }
	sockaddr* getSockAddr() { return (sockaddr*)&m_sockaddr; }

private:
	std::string m_ip;
	uint16_t m_port;
	sockaddr_in m_sockaddr;
};