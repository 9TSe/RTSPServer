#pragma once
#include "Log.h"
#include <functional>
using EventCallback = std::function<void(void*)>;

class TriggerEvent
{
public:
    TriggerEvent(void* arg);
    ~TriggerEvent();
    static TriggerEvent* createNew(void* arg);
    static TriggerEvent* createNew();

    void setTriggerCallback(EventCallback cb) { m_triggerCallback = cb; }
    void handleEvent();

private:
    EventCallback m_triggerCallback;
    void* m_arg;
};

class IOEvent
{
public:
    enum IOEventType
    {
        EVENT_NONE = 0,
        EVENT_READ = 1,
        EVENT_write = 2,
        EVENT_ERROR = 4
    };

    static IOEvent* createNew(int fd, void* arg);
    static IOEvent* createNew(int fd);
    IOEvent(int fd, void* arg);
    ~IOEvent();

    int getFd() const { return m_fd; }

    /*void Set_Arg(void* arg) { m_arg = arg; }
    void Set_ErrorCallback(EventCallback cb) { m_errorCallback = cb; }
    void Set_writeCallback(EventCallback cb) { m_writeCallback = cb; }
    void Enable_writeEvent() { m_event |= EVENT_write; }
    void Enable_ErrorEvent() { m_event |= EVENT_ERROR; }
    void Disable_ReadEvent() { m_event &= ~EVENT_READ; }
    void Disable_writeEvent() { m_event &= ~EVENT_write; }
    void Disable_ErrorEvent() { m_event &= ~EVENT_ERROR; }
    bool Is_NoneEvent() { return m_event == EVENT_NONE; }
    int Get_Event() { return m_event; }*/

    void setREvent(int revent) { m_revent = revent; }
    void setReadCallback(EventCallback cb) { m_readCallback = cb; }

    void enableReadEvent() { m_event |= EVENT_READ; }

    bool isReadEvent() const { return ((m_event & EVENT_READ) == EVENT_READ); }
    bool iswriteEvent() const { return ((m_event & EVENT_write) == EVENT_write); }
    bool isErrorEvent() const { return ((m_event & EVENT_ERROR) == EVENT_ERROR); }

    void handleEvent();

private:
    int m_fd;
    void* m_arg;
    int m_event;
    int m_revent;
    EventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_errorCallback;
};

class TimerEvent
{
public:
    TimerEvent(void* arg);
    ~TimerEvent();
    static TimerEvent* createNew(void* arg);
    static TimerEvent* createNew();

    void setTimerCallback(EventCallback cb) { m_timeCallback = cb; }
    bool handleEvent();

private:
    void* m_arg;
    EventCallback m_timeCallback;
    bool m_stop;
};