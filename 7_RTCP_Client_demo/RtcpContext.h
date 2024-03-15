#pragma once
#include "Rtcp.h"
#include <stddef.h>
#include <stdint.h>
#include <map>

class RtcpContext
{
public:
	virtual ~RtcpContext() = default;

	//sample_rate video stamp Sampling rate(采样率) video 90000, audio
	//bytes RTP_data length
	virtual void On_Rtp(uint16_t seq, uint32_t stamp, uint64_t ntp_stampMs,
		uint32_t sampleRate, size_t bytes);
	virtual void On_Rtcp(RtcpHeader* rtcp) = 0;
	virtual size_t Get_Lost(); //Packet loss
	virtual size_t Get_ExpectedPacket() const; //Deserve to receive(理应
	virtual RtcpSR* Create_RtcpSR(uint32_t rtcpSsrc);
	virtual RtcpRR* Create_RtcpRR(uint32_t rtcpSsrc, uint32_t rtpSsrc);
	virtual size_t Get_ExpectedPackets_Interval(); //between the last and this result
	virtual size_t Get_LostInterval();

protected:
	size_t m_bytes = 0;//rcv or send RTP
	size_t m_packets = 0; //RTP_Count
	uint32_t m_lastRtp_stamp = 0;
	uint64_t m_lastNtp_stampMs = 0;
};

class RtcpContextForSend : public RtcpContext
{
public:
	RtcpSR* Create_RtcpSR(uint32_t rtcpSsrc) override;
	void On_Rtcp(RtcpHeader* rtcp) override;
	uint32_t Get_Rtt(uint32_t ssrc) const; //get rtt(ms
private:
	std::map<uint32_t /*ssrc*/, uint32_t /*rtt*/> m_rtt;
	std::map<uint32_t /*lastSr_lsr*/, uint64_t /*ntpStamp*/> m_senderReport_ntp;
	std::map<uint32_t /*ssrc*/, uint64_t /*xr rrtr sys stamp*/> m_xrRtr_recvSysstamp;
	std::map<uint32_t /*ssrc*/, uint32_t /*lastRr*/> m_xrXrrtr_recvLastRr;
};