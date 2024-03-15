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

class RtcpContextForRecv : public RtcpContext
{
public:
	void On_Rtp(uint16_t seq, uint32_t stamp, uint64_t ntp_stampMs,
		uint32_t sampleRate, size_t bytes) override;
	void On_Rtcp(RtcpHeader* rtcp) override;
	RtcpRR* Create_RtcpRR(uint32_t rtcpSsrc, uint32_t rtpSsrc) override;
	size_t Get_ExpectedPacket() const override;
	size_t Get_ExpectedPackets_Interval() override;
	size_t Get_Lost() override;
	size_t Get_LostInterval() override;

private:
	double m_jitter = 0;
	uint16_t m_seqBase = 0; //first seq
	uint16_t m_seqMax = 0;
	uint16_t m_seqCycles = 0; //cycle num
	size_t m_lastCycle_packet = 0; //packet num
	uint16_t m_lastRtp_seq = 0;
	uint64_t m_lastRtp_sysStamp = 0; //ms
	size_t m_lastLost = 0; //num
	size_t m_lastExpected = 0;
	uint32_t m_lastSr_stamp = 0; //last sr timestamp
	uint64_t m_lastSr_ntpSys = 0; // sys stamp ms
};