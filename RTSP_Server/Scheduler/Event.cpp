#include "Event.h"


TriggerEvent::TriggerEvent(void* arg)
    :m_arg(arg)
    , m_triggerCallback(nullptr)
{
    LOGI("TriggerEvent()");
}

TriggerEvent* TriggerEvent::createNew()
{
    return new TriggerEvent(nullptr);
}

TriggerEvent* TriggerEvent::createNew(void* arg)
{
    return new TriggerEvent(arg);
}

TriggerEvent::~TriggerEvent()
{
    LOGI("~TriggerEvent");
}

void TriggerEvent::handleEvent()
{
    if (m_triggerCallback)
        m_triggerCallback(m_arg);
}



IOEvent::IOEvent(int fd, void* arg)
    : m_fd(fd)
    , m_arg(arg)
    , m_event(EVENT_NONE)
    , m_revent(EVENT_NONE)
    , m_readCallback(nullptr)
    , m_writeCallback(nullptr)
    , m_errorCallback(nullptr)
{
    LOGI("IOEvent(), m_fd = %d", m_fd);
}

IOEvent::~IOEvent()
{
    LOGI("~IOEvent()");
}

IOEvent* IOEvent::createNew(int fd, void* arg)
{
    if (fd < 0)
        return nullptr;
    return new IOEvent(fd, arg);
}

IOEvent* IOEvent::createNew(int fd)
{
    if (fd < 0) return nullptr;
    return new IOEvent(fd, nullptr);
}

void IOEvent::handleEvent()
{
    if (m_readCallback && (m_revent & EVENT_READ))
        m_readCallback(m_arg);
    if (m_writeCallback && (m_revent & EVENT_write))
        m_writeCallback(m_arg);
    if (m_errorCallback && (m_revent & EVENT_ERROR))
        m_errorCallback(m_arg);
}


TimerEvent::TimerEvent(void* arg)
    : m_arg(arg)
    , m_timeCallback(nullptr)
    , m_stop(false)
{
    LOGI("TimerEvent()");
}

TimerEvent::~TimerEvent()
{
    LOGI("~TimerEvent()");
}

TimerEvent* TimerEvent::createNew(void* arg)
{
    return new TimerEvent(arg);
}

TimerEvent* TimerEvent::createNew()
{
    return new TimerEvent(nullptr);
}

bool TimerEvent::handleEvent()
{
    if (m_stop)
        return true;
    if (m_timeCallback)
        m_timeCallback(m_arg);
    return false;
}