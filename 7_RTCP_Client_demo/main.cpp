#include "RtpClient.h"

int main()
{
	const char* serverIp = "127.0.0.1";
	const int serverPort = 11451;
	RtpClient rtpClient;
	rtpClient.Start(serverIp, serverPort);
	return 0;
}