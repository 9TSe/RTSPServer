#pragma once
#include <map>
#include "Event.h"

class Poller
{
public:
	virtual ~Poller();

	virtual bool Add_IOEvent(IOEvent* event) = 0;
	virtual bool Update_IOEvent(IOEvent* event) = 0;
	virtual bool Remove_IOEvent(IOEvent* event) = 0;
	virtual void Handle_Event() = 0;

protected:
	Poller();

	using IOEventMap = std::map<int, IOEvent*>;
	IOEventMap m_eventMap;
};