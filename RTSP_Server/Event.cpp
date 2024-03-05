#include <stdio.h>
#include "Event.h"
#include "Log.h"

TriggerEvent::TriggerEvent(void* arg)
	: m_arg(arg),
	m_triggerCallback(nullptr)
{
	LOGI("TriggerEvent()");
}

TriggerEvent::~TriggerEvent()
{
	LOGI("~TriggerEvent()");
}

TriggerEvent* TriggerEvent::Create_New(void* arg)
{
	return new TriggerEvent(arg);
}

TriggerEvent* TriggerEvent::Create_New()
{
	return new TriggerEvent(nullptr);
}

void TriggerEvent::Handle_Event()
{
	if (m_triggerCallback)
		m_triggerCallback(m_arg);
}




TimerEvent::TimerEvent(void* arg)
	: m_arg(arg),
	m_timeoutCallback(nullptr),
	m_isStop(false)
{
	LOGI("TimerEvent()");
}

TimerEvent::~TimerEvent()
{
	LOGI("~TimerEvent()");
}

TimerEvent* TimerEvent::Create_New(void* arg)
{
	return new TimerEvent(arg);
}

TimerEvent* TimerEvent::Create_New()
{
	return new TimerEvent(nullptr);
}

bool TimerEvent::Handle_Event()
{
	if (m_isStop)
		return m_isStop;
	if (m_timeoutCallback)
		m_timeoutCallback(m_arg);
	return m_isStop;
}

void TimerEvent::Stop()
{
	m_isStop = true;
}




IOEvent::IOEvent(int fd, void* arg)
	:m_fd(fd),
	m_arg(arg),
	m_event(EVENT_NONE),
	m_rEvent(EVENT_NONE),
	m_readCallback(nullptr),
	m_writeCallback(nullptr),
	m_errorCallback(nullptr)
{
	LOGI("IOEVENT() fd = %d", m_fd);
}

IOEvent::~IOEvent()
{
	LOGI("~IOEVENT() fd = %d", m_fd);
}

IOEvent* IOEvent::Create_New(int fd, void* arg)
{
	if (fd < 0)
		return nullptr;
	return new IOEvent(fd, arg);
}

IOEvent* IOEvent::Create_New(int fd)
{
	if (fd < 0)
		return nullptr;
	return new IOEvent(fd, nullptr);
}

void IOEvent::Handle_Event()
{
	if (m_readCallback && (m_rEvent & EVENT_READ))
		m_readCallback(m_arg);
	if (m_writeCallback && (m_rEvent & EVENT_WRITE))
		m_writeCallback(m_arg);
	if (m_errorCallback && (m_rEvent & EVENT_ERROR))
		m_errorCallback(m_arg);
}