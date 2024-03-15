//关于程序ffplay后速度和帧率都比较small的解释
//因为这个案例中读取h264码流的时候，每次读取一个nalu就会封装一次rtp包并发送，
//但事实上读取到一个nalu并不是对应一个视频帧，可能是sps，也可能是pps，也可能是其他类型的nalu，
//事实上这些nalu都不允许参与到休眠间隔时间的。所以sleep = 40，肯定会慢的，sleep = 20，肯定会快的。
//只有准确判断nalu类型，再进行封装rtp。 这个例子只是演示这样服务的原理，并没有处理这些细节。
#pragma warning( disable : 4996 )
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <iostream>
#include "rtp.h"
//#pragma warning( disable : 4996 )
#define H264_FILE_NAME   "D:\\ffmpeg\\learn\\test.h264"
#define SERVER_PORT      8554
#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE     (1024*1024)

static int CreateTcpSocket()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;
	int on = 1; 
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	return fd;
}

static int CreateUdpSocket()
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return -1;
	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	return fd;
}

static int BindSocketAddr(int fd, int port) //加上static封装函数,仅当前文件能够使用
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; //0 == 0.0.0.0(本机任意IP地址)
	if (bind(fd, (sockaddr*)&addr, sizeof(sockaddr)) < 0)
		return -1;
	return 0;
}

static int AcceptClient(int fd, char* ip, int* port)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	socklen_t len = sizeof(addr);
	int clientfd = accept(fd, (sockaddr*)&addr, &len);
	if (clientfd < 0)
		return -1;
	//32位地址转换为点分十进制字符串
	strcpy(ip, inet_ntoa(addr.sin_addr)); //inet_ntop更安全(包括线程安全),但是参数更多
	*port = ntohs(addr.sin_port); //转换为小端后存储
	return clientfd;
}


static void Handle_Option(char* buf, int cseq)
{
	sprintf(buf, "RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Public: OPTIONS, DESCRIBE, SETUP, PlAY\r\n"
		"\r\n",
		cseq);
}

static void Handle_Describe(char* buf, int cseq, char* url)
{
	char sdp[500];
	char local_ip[100];
	sscanf(url, "rtsp://%[^:]:", local_ip); //%[^:] 匹配除了:以外的任意字符
	sprintf(sdp, "v=0\r\n"
		"o=- 9%ld 1 IN IP4 %s\r\n"
		"t=0 0\r\n"
		"a=control:*\r\n"
		"m=video 0 RTP/AVP 96\r\n"
		"a=rtpmap:96 H264/90000\r\n"
		"a=control:track0\r\n",
		time(NULL), local_ip);

	sprintf(buf, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
		"Content-Base: %s\r\n"
		"Content-type: application/sdp\r\n"
		"Content-length: %zu\r\n\r\n"
		"%s",
		cseq, url, strlen(sdp), sdp);
}

static void Handle_Setup(char* buf, int cseq, int rtp_port)
{
	sprintf(buf, "RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
		"Session: 66334873\r\n"
		"\r\n",
		cseq, rtp_port, rtp_port + 1, SERVER_RTP_PORT, SERVER_RTCP_PORT);
}

static void Handle_Play(char* buf, int cseq)
{
	sprintf(buf, "RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Range: npt=0.000-\r\n"
		"Session: 66334873; timeout=10\r\n\r\n",
		cseq);
}
static inline int StartCode3(char* buf)
{
	//每个nalu单元都以这个开头
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) //00 00 01
		return 1;
	else
		return 0;
}

static inline int StartCode4(char* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) // 00 00 00 01
		return 1;
	else
		return 0;
}

static char* FindNextStartCode(char* buf, int len)
{
	if (len < 3) return nullptr;
	for (int i = 0; i < len - 3; ++i)
	{
		if (StartCode3(buf) || StartCode4(buf))
			return buf;
		++buf;
	}
	//if (StartCode3(buf))
		//return buf; 可能冗余了,运行test
	return nullptr;
}

static int GetFrameFromH264File(FILE* fp, char* frame, int size)
{
	int read_size, framesize;
	char* next_startcode;

	read_size = fread(frame, 1, size, fp); //从fp读取size个1大小的数据到frame,fp指针此时会移动
	if (!StartCode3(frame) && !StartCode4(frame)) //none startcode
		return -1;
	next_startcode = FindNextStartCode(frame + 3, read_size - 3); //返回起始码处的指针
	if (!next_startcode)
		return -1;
	else
	{
		framesize = (next_startcode - frame); //返回起始的是3还是4
		//由于前面的fread所以要将fp重新偏移回下一个frame的起始位置
		fseek(fp, framesize - read_size, SEEK_CUR); //fp指针相对于当前位置,偏移framesize - r_size
	}
	return framesize;
}

static int RtpSendH264Frame(int serverRtpSockfd, const char* ip, int16_t port,
							RtpPacket* rtpPacket, char* frame, uint32_t frameSize)
{
	int sendBytes = 0;
	int ret;
	//H.264由一个一个的NALU组成，每个NALU之间使用00 00 00 01或00 00 01分隔开，每个NALU的第一次字节都有特殊的含义
	uint8_t naluType = frame[0];
	std::cout << "frame size = " << frameSize << std::endl;

	if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包长：单一NALU单元模式
	{
		//*   0 1 2 3 4 5 6 7 8 9
		//*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		//*  |F|NRI|  Type   | a single NAL unit ... |
		//*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		memcpy(rtpPacket->payload, frame, frameSize); //将有效数据传输给有效载荷
		ret = RtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, frameSize);
		if (ret < 0)
			return -1;

		rtpPacket->rtpHeader.seq++; //序号加一
		sendBytes += ret;
		//0 00 11111 & 0 11 00111 0 00 00111  
		if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
			goto out;
	}
	else // nalu长度大于最大包场：分片模式
	{

		//*  0                   1                   2
		//*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
		//* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		//* | FU indicator  |   FU header   |   FU payload   ...  |
		//* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

		//*     FU Indicator
		//*    0 1 2 3 4 5 6 7
		//*   +-+-+-+-+-+-+-+-+
		//*   |F|NRI|  Type   |
		//*   +---------------+

		//*      FU Header
		//*    0 1 2 3 4 5 6 7
		//*   +-+-+-+-+-+-+-+-+
		//*   |S|E|R|  Type   |
		//*   +---------------+


		int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
		int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
		int i, pos = 1;

		// 发送完整的包
		for (i = 0; i < pktNum; i++)
		{
			// 0110 0000 | 0001 1100
			//分片打包载荷的第一个字节中,与NALU第一个字节的高三位相同,处理后, 0 ii 11100
			rtpPacket->payload[0] = (naluType & 0x60) | 28; //2,3位  第一位保持一直为0
			//0001 1111
			rtpPacket->payload[1] = naluType & 0x1F; //后5位,处理后 0 0 0 iiiii 代表既不是第一个也不是最后一个包

			if (i == 0) //第一包数据中的
				rtpPacket->payload[1] |= 0x80; // start 0 1 0 00000 : S标为1代表是第一个包
			else if (remainPktSize == 0 && i == pktNum - 1) //最后一个整包数据
				rtpPacket->payload[1] |= 0x40; // end 0 0 1 00000 : E标为1代表是最后一个包

			//frame就是NALU类型,第一个字节是header,给予了RTP格式的前两位信息后,RTP之后就是数据,这个数据就从NALU+1,跳过header得到
			memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PKT_SIZE); //由于前两个字节是载荷的信息不是有效数据
			ret = RtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, RTP_MAX_PKT_SIZE + 2);
			if (ret < 0)
				return -1;

			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
			pos += RTP_MAX_PKT_SIZE;
		}

		// 发送剩余的数据
		if (remainPktSize > 0)
		{
			rtpPacket->payload[0] = (naluType & 0x60) | 28;
			rtpPacket->payload[1] = naluType & 0x1F;
			rtpPacket->payload[1] |= 0x40; //end

			memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize + 2);
			ret = RtpSendPacketOverUdp(serverRtpSockfd, ip, port, rtpPacket, remainPktSize + 2);
			if (ret < 0)
				return -1;

			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
		}
	}


	rtpPacket->rtpHeader.timestamp += 90000 / 25; //  一秒定义为90000的单位 / 帧数 加时间戳
out:

	return sendBytes;

}

static void DoClient(int fd, const char* ip, int port)
{
	char method[40],url[100],version[40];
	int CSeq, client_Rtp_port, client_Rtcp_port;

	int server_Rtp_fd = -1;
	int server_Rtcp_fd = -1;

	char* r_buf = (char*)malloc(BUF_MAX_SIZE); //读缓冲区,读取客户端数据
	char* w_buf = (char*)malloc(BUF_MAX_SIZE); //写缓冲区,向客户端发送数据

	while (1)
	{
		int recv_len = recv(fd, r_buf, BUF_MAX_SIZE, 0);
		if (recv_len <= 0) break;
		r_buf[recv_len] = '\0';
		std::cout << std::endl << __FUNCTION__ << " r_buf  = " << r_buf << std::endl;
		const char* sepe = "\n";
		char* line = strtok(r_buf, sepe);
		while (line)
		{
			if (strstr(line, "OPTIONS") ||
				strstr(line, "DESCRIBE") ||
				strstr(line, "SETUP") ||
				strstr(line, "PLAY"))
			{
				if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3) //return success args num
				{
					std::cout << "sscanf method url version fail" << std::endl;
					exit(-1);
				}
			}
			else if (strstr(line, "CSeq"))
			{
				if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1)
				{
					std::cout << "sscanf CSeq" << std::endl;
					exit(-1);
				}
			}
			else if (!strncmp(line, "Transport:", strlen("Transport:"))) //suces
			{
				if (sscanf(line, "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n", &client_Rtp_port, &client_Rtcp_port) != 2)
				{
					std::cout << "sscanf 2 port fail" << std::endl;
					exit(-1);
				}
			}
			line = strtok(NULL, sepe);
			//nullptr
		}

		if (!strcmp(method, "OPTIONS"))
			Handle_Option(w_buf, CSeq); //no check
		else if (!strcmp(method, "DESCRIBE"))
			Handle_Describe(w_buf, CSeq, url);
		else if (!strcmp(method, "SETUP"))
		{
			Handle_Setup(w_buf, CSeq, client_Rtp_port);
			server_Rtp_fd = CreateUdpSocket();
			server_Rtcp_fd = CreateUdpSocket();
			if (server_Rtp_fd < 0 || server_Rtcp_fd < 0)
			{
				std::cout << "create server udp fail" << std::endl;
				exit(-1);
			}
			if(BindSocketAddr(server_Rtp_fd, SERVER_RTP_PORT) < 0||
				BindSocketAddr(server_Rtcp_fd, SERVER_RTCP_PORT) < 0)
			{
				std::cout << "bind server_fd fail" << std::endl;
				exit(-1);
			}
		}
		else if (!strcmp(method, "PLAY"))
			Handle_Play(w_buf, CSeq);
		else
			std::cout << "nb method : " << method << std::endl;

		std::cout << std::endl << __FUNCTION__ << "w_buf = " << w_buf << std::endl;
		send(fd, w_buf, strlen(w_buf), 0);

		if (!strcmp(method, "PLAY"))
		{
			int framesize, startcode;
			char* frame = (char*)malloc(500000);
			RtpPacket* rtp_packet = (RtpPacket*)malloc(500000);
			FILE* fp = fopen(H264_FILE_NAME, "rb"); //只读二进制
			if (!fp)
			{
				std::cout << "file fp fail" << std::endl;
				exit(-1);
			}
			RtpHeaderInit(rtp_packet, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, 0X88923423);
			std::cout << "start play\n" << "client ip:" << ip << std::endl << "client port:" << client_Rtp_port << std::endl;
			while (1)
			{
				framesize = GetFrameFromH264File(fp, frame, 500000); //获取的是当前 00至00的大小
				if (framesize < 0)
				{
					std::cout << "read " << H264_FILE_NAME << "end, framesize = " << framesize << std::endl;
					break;
				}
				if (StartCode3(frame))
					startcode = 3;
				else
					startcode = 4;

				framesize -= startcode; //减去开头的起始码,真正的数据大小
				RtpSendH264Frame(server_Rtp_fd, ip, client_Rtp_port, rtp_packet, frame + startcode, framesize);
				Sleep(30);
			}
			free(frame);
			free(rtp_packet);
			break;
		}

		memset(method, 0, sizeof(method));
		memset(url, 0, sizeof(url));
		CSeq = 0;
	} //next while

	closesocket(fd);
	if (server_Rtp_fd) {
		closesocket(server_Rtp_fd);
	}
	if (server_Rtcp_fd > 0) {
		closesocket(server_Rtcp_fd);
	}
	free(r_buf);
	free(w_buf);
}

int main()
{
	WSADATA winsock;
	if (WSAStartup(MAKEWORD(2, 2), &winsock) != 0)
	{
		std::cout << "winsock start error" << std::endl;
		return -1;
	}

	int server_fd = CreateTcpSocket();
	if (server_fd < 0)
	{
		WSACleanup();
		std::cout << "tcpsocket create fail" << std::endl;
		return -1;
	}

	if (BindSocketAddr(server_fd,SERVER_PORT) < 0)
	{
		std::cout << "bind fail" << std::endl;
		return -1;
	}

	if (listen(server_fd, 128) < 0)
	{
		std::cout << "listen fail" << std::endl;
		return -1;
	}

	std::cout << __FILE__ << " rtsp://127.0.0.1:" << SERVER_PORT << std::endl;

	while (1)
	{
		int client_fd;
		char client_ip[40];
		int client_port;
		client_fd = AcceptClient(server_fd, client_ip, &client_port);
		if (client_fd < 0)
		{
			std::cout << "accept fail" << std::endl;
			return -1;
		}
		std::cout << "accept client ip:" << client_ip << " port:" << client_port;
		DoClient(client_fd, client_ip, client_port);
	}
	closesocket(server_fd);
	return 0;
}