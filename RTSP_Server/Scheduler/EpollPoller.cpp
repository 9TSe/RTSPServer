#include "EpollPoller.h"

EpollPoller* EpollPoller::createNew()
{
	return new EpollPoller();
}

EpollPoller::EpollPoller()
	: m_epollFd(epoll_create1(0)) //create epoll tree
	, m_epollEvents(1024)
{
    if (m_epollFd == -1)
    {
        LOGE("Failed to create epoll file descriptor");
        return;
    }
	LOGI("Epoll init suc");
}

EpollPoller::~EpollPoller()
{
	close(m_epollFd);
}

bool EpollPoller::addIOEvent(IOEvent* event)
{
	return updateIOEvent(event);
}

bool EpollPoller::updateIOEvent(IOEvent* event)
{
    int fd = event->getFd();
    if (fd < 0) 
    {
        LOGE("fail to update Invalid fd = %d", fd);
        return false;
    }

    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;

    if (event->isReadEvent())
        ev.events |= EPOLLIN;
    if (event->iswriteEvent())
        ev.events |= EPOLLOUT;
    if (event->isErrorEvent())
        ev.events |= EPOLLERR;

    if (m_eventMap.find(fd) != m_eventMap.end()) 
    {
        if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev) == -1) 
        {
            LOGE("Failed to modify epoll event for fd = %d", fd);
            return false;
        }
    }
    else 
    {
        if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) 
        {
            LOGE("Failed to add epoll event for fd = %d", fd);
            return false;
        }
        m_eventMap[fd] = event;
    }

    return true;
}

bool EpollPoller::removeIOEvent(IOEvent* event)
{
    int fd = event->getFd();
    if (fd < 0) 
    {
        LOGE("fail to remove Invalid fd = %d", fd);
        return false;
    }

    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) == -1) 
    {
        /*LOGE("Failed to delete epoll event for fd = %d", fd);
        return false;*/
        switch (errno) {
        case EBADF:
            fprintf(stderr, "Invalid file descriptor: %d\n", fd);
            break;
        case ENOENT:
            fprintf(stderr, "File descriptor %d not registered in epoll instance\n", fd);
            break;
        case EPERM:
            fprintf(stderr, "Operation not permitted for file descriptor: %d\n", fd);
            break;
        default:
            fprintf(stderr, "Unknown error occurred while deleting fd: %d\n", fd);
            break;
        }
        return false;
    }
    m_eventMap.erase(fd);
    return true;
}



void EpollPoller::handleEvent()
{
    int numEvents = epoll_wait(m_epollFd, &m_epollEvents[0], static_cast<int>(m_epollEvents.size()), -1);
    if (numEvents < 0) 
    {
        LOGE("Error in epoll_wait");
        return;
    }

    for (int i = 0; i < numEvents; ++i) 
    {
        int fd = m_epollEvents[i].data.fd;
        int present_event = 0;

        if (m_epollEvents[i].events & EPOLLIN)
            present_event |= IOEvent::EVENT_READ;
        if (m_epollEvents[i].events & EPOLLOUT)
            present_event |= IOEvent::EVENT_write;
        if (m_epollEvents[i].events & EPOLLERR)
            present_event |= IOEvent::EVENT_ERROR;

        auto it = m_eventMap.find(fd);
        if (it != m_eventMap.end() && present_event != 0) 
        {
            it->second->setREvent(present_event);
            it->second->handleEvent();
        }
    }
}