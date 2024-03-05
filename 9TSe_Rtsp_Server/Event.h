#pragma once
using EventCallback = void(*)(void*);
//typedef void (*EventCallback)(void*);

class TriggerEvent
{
public:
	TriggerEvent(void* arg);
	~TriggerEvent();

	static TriggerEvent* Create_New(void* arg);
	static TriggerEvent* Create_New();

	void Set_Arg(void* arg) { m_arg = arg; }
	void Set_TriggerCallback(EventCallback callback) { m_triggerCallback = callback; }
	void Handle_Event();

private:
	void* m_arg;
	EventCallback m_triggerCallback;
};

class TimerEvent
{
public:
	TimerEvent(void* arg);
	~TimerEvent();

	static TimerEvent* Create_New(void* arg);
	static TimerEvent* Create_New();

	void Set_Arg(void* arg) { m_arg = arg; }
	void Set_Timeout_Callback(EventCallback callback) { m_timeoutCallback = callback; }
	bool Handle_Event();
	void Stop();

private:
	void* m_arg;
	EventCallback m_timeoutCallback;
	bool m_isStop;
};

class IOEvent
{
public:
	enum IOEventType
	{
		EVENT_NONE = 0,
		EVENT_READ = 1,
		EVENT_WRITE = 2,
		EVENT_ERROR = 4,
	};

	IOEvent(int fd, void* arg);
	~IOEvent();

	static IOEvent* Create_New(int fd, void* arg);
	static IOEvent* Create_New(int fd);

	int Get_Fd() const { return m_fd; }
	int Get_Event() const { return m_event; }
	void Set_REvent(int event) { m_rEvent = event; }
	void Set_Arg(void* arg) { m_arg = arg; }

	void Set_ReadCallback(EventCallback callback) { m_readCallback = callback; }
	void Set_WriteCallback(EventCallback callback) { m_writeCallback = callback; }
	void Set_ErrorCallback(EventCallback callback) { m_errorCallback = callback; }

	void Enable_Read_Handling() { m_event |= EVENT_READ; }
	void Enable_Write_Handling() { m_event |= EVENT_WRITE; }
	void Enable_Error_Handling() { m_event |= EVENT_ERROR; }
	void Disable_Read_Handling() { m_event &= ~EVENT_READ; }
	void Disable_Write_Handling() { m_event &= ~EVENT_WRITE; }
	void Disable_Error_Handling() { m_event &= ~EVENT_ERROR; }

	bool IsNone_Handling()const { return m_event == EVENT_NONE; }
	bool IsRead_Handling()const { return (m_event & EVENT_READ) != 0; }
	bool IsWrite_Handling()const { return (m_event & EVENT_WRITE) != 0; }
	bool IsError_Handling()const { return (m_event & EVENT_ERROR) != 0; }

	void Handle_Event();

private:
	int m_fd;
	void* m_arg;
	int m_event;
	int m_rEvent;
	EventCallback m_readCallback;
	EventCallback m_writeCallback;
	EventCallback m_errorCallback;
};