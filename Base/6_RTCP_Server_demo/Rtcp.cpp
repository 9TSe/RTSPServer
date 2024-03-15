#include "Rtcp.h"
#include <stdio.h>
#include <iostream>

#if defined(_MSC_VER) //microsoft C version
#include <BaseTsd.h> //Provide a custom name for the primitive type name
typedef SSIZE_T ssize_t; //(sign int) base diffrent OS 
#endif

std::vector<RtcpHeader*> RtcpHeader::Load_FromBytes(char* data, size_t len)
{
	std::vector<RtcpHeader*> ret;
	ssize_t remain = len;
	char* ptr = data;
	while (remain > (ssize_t)sizeof(RtcpHeader))
	{
		RtcpHeader* rtcp = (RtcpHeader*)ptr;
		auto rtcp_len = rtcp->Get_Size();
		if (remain < (ssize_t)rtcp_len)
		{
			std::cout << "unable rtcppacket, decalred length exceeds the actual length" << std::endl;
			break;
		}
		try
		{
			rtcp->NetTo_Host(rtcp_len);
			ret.emplace_back(rtcp);
		}
		catch (std::exception& ex)
		{
			//unable parse
			std::cout << ex.what() << ", length:" << rtcp_len << std::endl;
		}
		ptr += rtcp_len;
		remain -= rtcp_len;
	}
	return ret;
}

void RtcpHeader::NetTo_Host(size_t len)
{
	switch ((RtcpType)pt)
	{
	case RtcpType::RTCP_SR:
	{
		RtcpSR* sr = (RtcpSR*)this;
		sr->NetTo_Host(len);
		break;
	}
	case RtcpType::RTCP_RR:
	{
		RtcpRR* rr = (RtcpRR*)this;
		rr->NetTo_Host(len);
		break;
	}
	default: {
		std::cout << "unprocessed rtcppacket:" << this->pt << std::endl;
		}
	}
}

size_t RtcpHeader::Get_Size() const
{
	return (1 + ntohs(length)) << 2;
}

size_t RtcpHeader::Get_PaddingSize() const
{
	if (!padding)
		return 0;
	return ((uint8_t*)this)[Get_Size() - 1];
}

void RtcpHeader::Set_Size(size_t size)
{
	length = htons((uint16_t)((size >> 2) - 1));
}

void ReportItem::NetTo_Host()
{
	ssrc = ntohl(ssrc);
	cumulative = ntohl(cumulative) >> 8;
	seq_cycles = ntohs(seq_cycles);
	seq_max = ntohs(seq_max);
	jitter = ntohl(jitter);
	last_sr_stamp = ntohl(last_sr_stamp);
	delay_since_last_sr = ntohl(delay_since_last_sr);
}

void RtcpSR::NetTo_Host(size_t size)
{
	static const size_t kminSize = sizeof(RtcpSR) - sizeof(items);
	ssrc = ntohl(ssrc);
	ntpmsw = ntohl(ntpmsw);
	ntplsw = ntohl(ntplsw);
	rtpts = ntohl(rtpts);
	packet_count = ntohl(packet_count);
	octet_count = ntohl(octet_count);

	ReportItem* ptr = &items;
	int item_count = 0;
	for (int i = 0; i < (int)report_count && (char*)(ptr)+sizeof(ReportItem) <= (char*)(this) + size; ++i)
	{
		ptr->NetTo_Host();
		++ptr;
		++item_count;
	}
}


void RtcpSR::Set_NTPStamp(timeval tv)
{
	ntpmsw = htonl(tv.tv_sec + 0x03AA7E80); //1970 - 1900
	ntplsw = htonl((uint32_t)((double)tv.tv_usec * (double)(((uint64_t)1) << 32) * 1.0e-6)); //<<?
}

void RtcpSR::Set_NTPStamp(uint64_t unix_stamp_ms)
{
	timeval tv;
	tv.tv_sec = unix_stamp_ms / 1000;
	tv.tv_usec = (unix_stamp_ms % 1000) * 1000;
	Set_NTPStamp(tv);
}

std::vector<ReportItem*> RtcpRR::Get_ItemList()
{
	std::vector<ReportItem*> ret;
	ReportItem* ptr = &items;
	for (int i = 0; i < (int)report_count; ++i)
	{
		ret.emplace_back(ptr);
		++ptr;
	}
	return ret;
}

void RtcpRR::NetTo_Host(size_t size)
{
	static const size_t kminSize = sizeof(RtcpRR) - sizeof(items);
	ssrc = ntohl(ssrc);
	ReportItem* ptr = &items;
	int item_count = 0;
	for (int i = 0; i < (int)report_count && (char*)(ptr)+sizeof(ReportItem) <= (char*)(this) + size; ++i)
	{
		ptr->NetTo_Host();
		++ptr;
		++item_count;
	}
}