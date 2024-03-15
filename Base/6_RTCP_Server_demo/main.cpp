#include "RtpServer.h"

int main()
{
	const char* ip = "127.0.0.1";
	uint16_t port = 11451;

	RtpServer rtpServer;
	rtpServer.Start(ip, port);
	return 0;
}