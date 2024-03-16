#include "RtspClient.h"

int main(int argc, const char* argv[])
{
	printf("main argc=%d\n", argc);
	const char* transport = "tcp";
	const char* url = "rtsp://127.0.0.1:554/live/test";
	RtspClient rtspClient(transport, url);
	if (rtspClient.Connect_Server())
		rtspClient.Start_Cmd();
	return 0;
}