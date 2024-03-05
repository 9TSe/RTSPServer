#include "InetAddress.h"

Ipv4Address::Ipv4Address()
{}

Ipv4Address::Ipv4Address(std::string ip, uint16_t port)
	:m_ip(ip),
	m_port(port)
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	m_addr.sin_port = htons(port);
}

void Ipv4Address::Set_Addr(std::string ip, uint16_t port)
{
	m_ip = ip;
	m_port = port;
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	m_addr.sin_port = htons(port);
}

std::string Ipv4Address::Get_Ip()
{
	return m_ip;
}

uint16_t Ipv4Address::Get_Port()
{
	return m_port;
}

sockaddr* Ipv4Address::Get_Addr()
{
	return (sockaddr*)&m_addr;
}