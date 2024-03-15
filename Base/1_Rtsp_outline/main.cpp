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
#include <string>
#pragma comment(lib, "ws2_32.lib") //���ӽ׶ν�ָ���Ŀ��ļ� ws2_32.lib ���뵽��ǰ��Ŀ�С�(�׽��ֱ�̵��ļ�)
#include <stdint.h>
#include <iostream>
#pragma warning( disable : 4996 )
#define SERVER_PORT      8554
#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533

int CreateTcpSocket()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;
	int on = 1; //��setsockopt�������1�ͻ�����ַ��ռ�õ�����,������������������

	//SOL_SOCKET ��һ����������ʾ�׽��ּ����ѡ������������׽������ͣ��������ض��Ĵ���Э��
	//���������ֵ,���ܻ����,����IPV6,4,TCP,UDPЭ����ص�ѡ��
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
	return fd;
}

static int BindSocketAddr(int fd) //����static��װ����,����ǰ�ļ��ܹ�ʹ��
{
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = INADDR_ANY; //0 == 0.0.0.0(��������IP��ַ)
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
	//32λ��ַת��Ϊ���ʮ�����ַ���
	strcpy(ip, inet_ntoa(addr.sin_addr)); //inet_ntop����ȫ(�����̰߳�ȫ),���ǲ�������
	*port = ntohs(addr.sin_port); //ת��ΪС�˺�洢
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
	sscanf(url, "rtsp://%[^:]:", local_ip); //%[^:] ƥ�����:����������ַ�
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
		cseq,url,strlen(sdp),sdp);
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

static void DoClient(int fd, const char* ip, int port)
{
	//�ͻ��˿��ܷ���������Ϣ(����) DESCRIBE rtsp ://127.0.0.1:8554/stream1 RTSP/1.0
	char method[40]; //DESCRIBE
	char url[100]; //rtsp://127.0.0.1:8554/stream1
	char version[40]; //RTSP/1.0

	int CSeq; //�����е����к�
	int client_Rtp_port;
	int client_Rtcp_port;

	char* r_buf = (char*)malloc(10000); //��������,��ȡ�ͻ�������
	char* w_buf = (char*)malloc(10000); //д������,��ͻ��˷�������

	while (1) //more reuqest
	{
		int recv_len = recv(fd, r_buf, 2000, 0); //�ӿͻ���(fd)�ж�ȡ���2000�����ݵ�r_buf;
		if (recv_len <= 0) //return readed len
			break;
		r_buf[recv_len] = '\0';
		std::cout << std::endl << __FUNCTION__ << " r_buf  = " << r_buf << std::endl;
		const char* sepe = "\n";
		char* line = strtok(r_buf, sepe);
		while (line) //only one request do
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
			Handle_Setup(w_buf, CSeq, client_Rtp_port);
		else if (!strcmp(method, "PLAY"))
			Handle_Play(w_buf, CSeq);
		else
			std::cout << "nb method : " << method << std::endl;

		std::cout << std::endl << __FUNCTION__ << "w_buf = " << w_buf << std::endl;
		send(fd, w_buf, strlen(w_buf), 0);

		if (!strcmp(method, "PLAY"))
		{
			std::cout << "start play\n" << "client ip:" << ip << std::endl << "client port:" << client_Rtp_port << std::endl;
			while (1)
			{
				//...
				Sleep(40);
			}
			break;
		}

		memset(method, 0, sizeof(method));
		memset(url, 0, sizeof(url));
		CSeq = 0;
	} //next while

	closesocket(fd);
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

	if (BindSocketAddr(server_fd) < 0)
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