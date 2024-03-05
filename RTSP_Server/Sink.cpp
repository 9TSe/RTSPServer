#include "Sink.h"
#include "SocketsOps.h"
#include "Log.h"

Sink::Sink(UsageEnvironment* env, MediaSource* mediasource, int payloadtype)
	:m_mediaSource(mediasource),
	m_env(env),
	m_csrcLen(0),
	m_extension(0),
	m_padding(0),
	m_version(RTP_VERSION),
	m_payloadType(payloadtype),
	m_marker(0),
	m_seq(0),
	m_ssrc(rand()),
	m_timeStamp(0),
	m_timerId(0),
	m_sessionSendpacket_callback(nullptr),
	m_arg1(nullptr),
	m_arg2(nullptr)
{
	LOGI("Sink()");
	m_timerEvent = TimerEvent::Create_New(this);
	m_timerEvent->Set_Timeout_Callback(Callback_Timeout);
}

Sink::~Sink()
{
	LOGI("~Sink()");
	delete m_timerEvent;
	delete m_mediaSource;
}

void Sink::Callback_Timeout(void* arg)
{
	Sink* sink = (Sink*)arg;
	sink->Handle_Timeout();
}

void Sink::Stop_TimeEvent()
{
	m_timerEvent->Stop();
}

void Sink::Set_SessionCallback(SessionSendPacketCallback callback, void* arg1, void* arg2)
{
	m_sessionSendpacket_callback = callback;
	m_arg1 = arg1;
	m_arg2 = arg2;
}

void Sink::Send_RtpPacket(RtpPacket* packet)
{
	RtpHeader* rtpheader = packet->m_rtpHeader;
	rtpheader->csrcLen = m_csrcLen;
	rtpheader->extension = m_extension;
	rtpheader->padding = m_padding;
	rtpheader->version = m_version;
	rtpheader->payloadType = m_payloadType;
	rtpheader->marker = m_marker;
	rtpheader->seq = htons(m_seq);
	rtpheader->timestamp = htonl(m_timeStamp);
	rtpheader->ssrc = htonl(m_ssrc);

	if (m_sessionSendpacket_callback)
		m_sessionSendpacket_callback(m_arg1, m_arg2, packet, PacketType::RTPPACKET);
}

void Sink::Handle_Timeout()
{
	MediaFrame* frame = m_mediaSource->GetFrame_From_OutputQueue();
	if (!frame)
		return;
	this->Send_Frame(frame); //wait son achive
	m_mediaSource->PutFrame_To_InputQueue(frame);
	//将使用过的frame插入输入队列后，加入一个子线程task，从文件中读取数据再次将输入写入到frame
}

void Sink::Run_Every(int interval)
{
	m_timerId = m_env->Scheduler()->Add_TimedEvent_RunEvery(m_timerEvent, interval);
}