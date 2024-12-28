#include "InetAddress.h"

IPV4Address::IPV4Address(){}

IPV4Address::IPV4Address(std::string ip, uint16_t port)
	:m_ip(ip)
	,m_port(port)
{
	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons(m_port);
	m_sockaddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
}

void IPV4Address::Set_Address(std::string ip, uint16_t port)
{
	m_ip = ip;
	m_port = port;
	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons(m_port);
	m_sockaddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
}