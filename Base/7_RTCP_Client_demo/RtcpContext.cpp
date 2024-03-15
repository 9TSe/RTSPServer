#include "RtcpContext.h"
#include "Utils.h"
#include <stdio.h>
#include <string.h>

void RtcpContext::On_Rtp(uint16_t seq, uint32_t stamp, uint64_t ntp_stampMs,
	uint32_t sampleRate, size_t bytes)
{
	++m_packets;
	m_bytes += bytes;
	m_lastRtp_stamp = stamp;
	m_lastNtp_stampMs = ntp_stampMs;
}

size_t RtcpContext::Get_ExpectedPacket()const
{
	throw std::runtime_error("unrealized, rtpsender cannot collect statistics on the number of packets receivable");
}

size_t RtcpContext::Get_ExpectedPackets_Interval()
{
	throw std::runtime_error("unrealized, rtpsender cannot collect statistics on the number of packets receivable");
}

size_t RtcpContext::Get_Lost()
{
	throw std::runtime_error("unrealized, rtpsender cannot collect statistics on the number of packets receivable");
}

size_t RtcpContext::Get_LostInterval()
{
	throw std::runtime_error("unrealized, rtpsender cannot collect statistics on the number of packets receivable");
}

RtcpSR* RtcpContext::Create_RtcpSR(uint32_t rtcpSsrc)
{
	throw std::runtime_error("unrealized, rtp reciver attempts to send a sr packet");
}

RtcpRR* RtcpContext::Create_RtcpRR(uint32_t rtcpSsrc, uint32_t rtpSsrc)
{
	throw std::runtime_error("unrealized, rtp sender attempted to send a rr packet");
}



void RtcpContextForSend::On_Rtcp(RtcpHeader* rtcp)
{
	switch ((RtcpType)rtcp->pt)
	{
	case RtcpType::RTCP_RR:
	{
		//RtcpRR* rtcpRr = (RtcpRR*)rtcp;
		auto rtcpRr = (RtcpRR*)rtcp;
		for (auto item : rtcpRr->Get_ItemList())
		{
			if (!item->last_sr_stamp)
				continue;
			auto it = m_senderReport_ntp.find(item->last_sr_stamp);
			if (it == m_senderReport_ntp.end())
				continue;

			auto msInc = Get_CurrentMillisecond() - it->second; //发送sr 到 收到rr 间的时间戳增量
			auto delayMs = (uint64_t)item->delay_since_last_sr * 1000 / 65536; //rtpReciver get sr, reply rr delay
			auto rtt = (int)(msInc - delayMs);
			if (rtt >= 0) //coursely
				m_rtt[item->ssrc] = rtt;
		}
		break;
	}
	default:
		break;
	}
}

uint32_t RtcpContextForSend::Get_Rtt(uint32_t ssrc) const
{
	auto it = m_rtt.find(ssrc);
	if (it == m_rtt.end())
		return 0;
	return it->second;
}

RtcpSR* RtcpContextForSend::Create_RtcpSR(uint32_t rtcpSsrc)
{
	size_t itemCount = 0; //SR包扩展 0 个reportitem
	auto realSize = sizeof(RtcpRR) - sizeof(ReportItem) + itemCount * sizeof(ReportItem);
	auto bytes = AlignSize(realSize);
	auto rtcp = new RtcpSR;
	SetUp_Header(rtcp, RtcpType::RTCP_SR, itemCount, bytes);
	SetUp_Padding(rtcp, bytes - realSize);
	rtcp->ntpmsw = 0;
	rtcp->ntplsw = 0;
	rtcp->Set_NTPStamp(m_lastNtp_stampMs);
	rtcp->rtpts = htonl(m_lastRtp_stamp);
	rtcp->ssrc = htonl(rtcpSsrc);
	rtcp->packet_count = htonl((uint32_t)m_packets);
	rtcp->octet_count = htonl((uint32_t)m_bytes);

	//record last send senderReport msg, to after compute rtt
	auto last_sr_lsr = ((ntohl(rtcp->ntpmsw) & 0xffff) << 16) | ((ntohl(rtcp->ntplsw) >> 16) & 0xffff);
	m_senderReport_ntp[last_sr_lsr] = Get_CurrentMillisecond();
	if (m_senderReport_ntp.size() >= 5)
		m_senderReport_ntp.erase(m_senderReport_ntp.begin());

	return rtcp;
}