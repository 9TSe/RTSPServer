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



RtcpRR* RtcpContextForRecv::Create_RtcpRR(uint32_t rtcpSsrc, uint32_t rtpSsrc)
{
	size_t itemCount = 1;
	auto realSize = sizeof(RtcpRR) - sizeof(ReportItem) + itemCount * sizeof(ReportItem);
	size_t bytes = AlignSize(realSize);
	RtcpRR* rtcp = new RtcpRR;
	SetUp_Header(rtcp, RtcpType::RTCP_RR, itemCount, bytes);
	SetUp_Padding(rtcp, bytes - realSize);
	rtcp->ssrc = htonl(rtcpSsrc);
	ReportItem* item = (ReportItem*)&rtcp->items;
	item->ssrc = htonl(rtpSsrc);
	
	uint8_t fraction = 0;
	size_t expectedInterval = Get_ExpectedPackets_Interval();
	if (expectedInterval)
		fraction = uint8_t(Get_LostInterval() << 8 / expectedInterval); // ?

	item->fraction = fraction;
	item->cumulative = htonl(uint32_t(Get_Lost())) >> 8;
	item->seq_cycles = htons(m_seqCycles);
	item->seq_max = htons(m_seqMax);
	item->jitter = htonl(uint32_t(m_jitter));
	item->last_sr_stamp = htonl(m_lastSr_stamp);
	
	auto delay = Get_CurrentMillisecond() - m_lastSr_ntpSys; //ms
	auto dlsr = (uint32_t)(delay / 1000.0f * 65536); // 1/65536 unite (为单位
	item->delay_since_last_sr = htonl(m_lastSr_stamp ? dlsr : 0);
	return rtcp;
}

void RtcpContextForRecv::On_Rtp(uint16_t seq, uint32_t stamp, uint64_t ntp_stampMs,
	uint32_t sampleRate, size_t bytes)
{
	size_t sysStamp = Get_CurrentMillisecond();
	if (m_lastRtp_sysStamp)
	{
		//Calculate the timestamp jitter value(计算时间戳抖动值
		double diff = double((int64_t(sysStamp) -
			int64_t(m_lastRtp_sysStamp)) * (sampleRate / double(1000.0)) -
			(int64_t(stamp) - int64_t(m_lastRtp_stamp)));
		if (diff < 0)
			diff = -diff;
		m_jitter += (diff - m_jitter) / 16.0;
	}
	else
		m_jitter;
	if (m_lastRtp_seq > 0xff00 && seq < 0xff && (!m_seqCycles || m_packets - m_lastCycle_packet > 0x1fff))
	{
		//未发生回环或距上次回环间隔超过0x1fff包认为是回环
		++m_seqCycles;
		m_lastCycle_packet = m_packets;
		m_seqMax = seq;
	}
	else if (seq > m_seqMax)
		m_seqMax = seq;

	if (!m_seqBase)
		m_seqBase = seq;
	else if (!m_seqCycles && seq < m_seqBase)
		m_seqBase = seq;

	m_lastRtp_seq = seq;
	m_lastRtp_sysStamp = sysStamp;

	RtcpContext::On_Rtp(seq, stamp, ntp_stampMs, sampleRate, bytes);
}

void RtcpContextForRecv::On_Rtcp(RtcpHeader* rtcp)
{
	switch ((RtcpType)rtcp->pt)
	{
	case RtcpType::RTCP_SR:
	{
		RtcpSR* rtcpSr = (RtcpSR*)rtcp;
		//  last SR timestamp(LSR) : 32 bits
		//	The middle 32 bits out of 64 in the NTP timestamp(as explained in
		//	Section 4) received as part of the most recent RTCP sender report
		//	(SR) packet from source SSRC_n.If no SR has been received yet,
		//	the field is set to zero.
		m_lastSr_stamp = ((rtcpSr->ntpmsw & 0xffff) << 16) | ((rtcpSr->ntplsw >> 16) & 0xffff);
		m_lastSr_ntpSys = Get_CurrentMillisecond();
		break;
	}
	default:
		break;
	}
}

size_t RtcpContextForRecv::Get_ExpectedPacket() const
{
	return (m_seqCycles << 16) + m_seqMax - m_seqBase + 1;
}

size_t RtcpContextForRecv::Get_ExpectedPackets_Interval()
{
	size_t expected = Get_ExpectedPacket();
	size_t ret = expected - m_lastExpected;
	m_lastExpected = expected;
	return ret;
}

size_t RtcpContextForRecv::Get_Lost()
{
	return Get_ExpectedPacket() - m_packets;
}

size_t RtcpContextForRecv::Get_LostInterval()
{
	size_t lost = Get_Lost();
	size_t ret = lost - m_lastLost;
	m_lastLost = lost;
	return ret;
}