#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <sstream>
#include <WinSock2.h>

#ifndef PACKED
	#ifndef _WIN32
	#define PACKED __attribute__((packed))
	#else
	#define PACKED
	#endif 
#endif // !PACKED

#define RTCP_PT_MAP(XX)		\
	XX(RTCP_FIR, 192)		\
	XX(RTCP_NACK, 193)		\
	XX(RTCP_SMPTETC, 194)	\
	XX(RTCP_IJ, 195)		\
	XX(RTCP_SR, 200)		\
	XX(RTCP_RR, 201)		\
	XX(RTCP_SDES, 202)		\
	XX(RTCP_BYE, 203)		\
	XX(RTCP_APP, 204)		\
	XX(RTCP_RTPFB, 205)		\
	XX(RTCP_PSFB, 206)		\
	XX(RTCP_XR, 207)		\
	XX(RTCP_AVB, 208)		\
	XX(RTCP_RSI, 209)		\
	XX(RTCP_TOKEN, 210)		

enum class RtcpType : uint8_t
{
#define XX(key,value) key = value,
	RTCP_PT_MAP(XX)
#undef XX 
};
// 宏转换为-> 
//RTCP_FIR = 192,
//RTCP_NACK = 193, ...

class RtcpHeader
{
public:
	uint32_t report_count	: 5;
	uint32_t padding		: 1;
	uint32_t version		: 2;
	uint32_t pt				: 8; //rtcptype
private:
	uint32_t length			: 16;//rtcp_length

public:
	//parse rtcp Network byte order(big-endian) -> Host byte order(small-endian), 
	//return rtcpheader vector
	static std::vector<RtcpHeader*> Load_FromBytes(char* data, size_t size);

	size_t Get_Size() const;

	//get append length
	size_t Get_PaddingSize() const;

	void Set_Size(size_t size);

private:
	void NetTo_Host(size_t size);
};


class ReportItem
{
public:
	friend class RtcpSR;
	friend class RtcpRR;

	uint32_t ssrc;
	uint32_t fraction : 8;//Fraction Lost
	uint32_t cumulative : 24; //cumulative number of packets lost

	uint16_t seq_cycles; //Sequence number cycles count (序列号循环计数
	uint16_t seq_max;	 //highest sequence number received (序列最大值

	uint32_t jitter; //Interarrival jitter (接受抖动,RTP数据包接收时间的统计方差估计
	uint32_t last_sr_stamp; 
	//Last SR,LSR timestamp, NTP timestamp, (ntpmsw & 0xffff) << 16 | (ntplsw >> 16) & 0xffff)
	//上次SR时间戳，取最近收到的SR包中的NTP时间戳的中间32比特。如果目前还没收到SR包，则该域清零

	uint32_t delay_since_last_sr; //Delay since last SR,DLSR(上次SR以来的延时, 上次收到SR包到发送本报告的延时

private:
	void NetTo_Host();
} PACKED;

class RtcpSR : public RtcpHeader
{
public:
	friend class RtcpHeader;
	uint32_t ssrc;
	uint32_t ntpmsw = 0; //ntp timestamp MSW(in second)
	uint32_t ntplsw = 0; //ntp timestamp LSW(in picosecond) 微微秒
	uint32_t rtpts = 0;  //rtp timestamp With NTP timestamp correspondence
	uint32_t packet_count = 0; //sender RTP packet count
	uint32_t octet_count = 0; //send RTP octet count (字节数
	ReportItem items; //可扩展,可需要

public:
	void Set_NTPStamp(timeval tv);
	void Set_NTPStamp(uint64_t unix_stamp_ms);
private:
	void NetTo_Host(size_t size);
} PACKED;

class RtcpRR : public RtcpHeader
{
public:
	friend class RtcpHeader;
	uint32_t ssrc;
	ReportItem items;

public:
	std::vector<ReportItem*> Get_ItemList();
private:
	void NetTo_Host(size_t size);
} PACKED;


static size_t AlignSize(size_t bytes)
{
	return (size_t)((bytes + 3) >> 2) << 2; // ?
}

static void SetUp_Header(RtcpHeader* rtcp, RtcpType type, size_t reportCount, size_t totalBytes)
{
	rtcp->version = 2;
	rtcp->padding = 0;
	if (reportCount > 0x1f) //32
	{
		printf("rtcp report_count max 31, now : %lld", reportCount);
		return;
	}
	rtcp->report_count = reportCount;
	rtcp->pt = (uint8_t)type;
	rtcp->Set_Size(totalBytes);
}

static void SetUp_Padding(RtcpHeader* rtcp, size_t paddingSize)
{
	if (paddingSize)
	{
		rtcp->padding = 1;
		((uint8_t*)rtcp)[rtcp->Get_Size() - 1] = paddingSize & 0xff;
	}
	else
		rtcp->padding = 0;
}