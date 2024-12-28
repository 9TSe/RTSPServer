#include "EpollPoller.h"
#include "Event.h"
#include "Log.h"

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
        LOG_CORE_ERROR("Failed to create epoll file descriptor");
        return;
    }
	LOG_CORE_INFO("Epoll init suc");
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
        LOG_CORE_ERROR("fail to update Invalid fd = %d", fd);
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
            LOG_CORE_ERROR("Failed to modify epoll event for fd = %d", fd);
            return false;
        }
    }
    else 
    {
        if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) == -1) 
        {
            LOG_CORE_ERROR("Failed to add epoll event for fd = %d", fd);
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
        LOG_CORE_ERROR("fail to remove Invalid fd = %d", fd);
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
        LOG_CORE_ERROR("Error in epoll_wait");
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

// -----

std::shared_ptr<BoostPoller> BoostPoller::createNew() {
    return std::make_shared<BoostPoller>();
}

BoostPoller::BoostPoller() {
    LOG_CORE_INFO("BoostPoller initialized");
}

bool BoostPoller::addIOEvent(IOEvent* event) {
    return updateIOEvent(event);
}

bool BoostPoller::updateIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        LOG_CORE_ERROR("Invalid fd = %d", fd);
        return false;
    }

    auto stream = std::make_shared<boost::asio::posix::stream_descriptor>(m_ioContext, fd);

    if (event->isReadEvent()) {
        stream->async_read_some(boost::asio::null_buffers(),
            [event](const boost::system::error_code& ec, std::size_t) {
                if (!ec) {
                    event->setREvent(IOEvent::EVENT_READ);
                    event->handleEvent();
                }
            });
    }

    if (event->iswriteEvent()) {
        stream->async_write_some(boost::asio::null_buffers(),
            [event](const boost::system::error_code& ec, std::size_t) {
                if (!ec) {
                    event->setREvent(IOEvent::EVENT_write);
                    event->handleEvent();
                }
            });
    }

    m_eventMap[fd] = stream;
    return true;
}

bool BoostPoller::removeIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0 || m_eventMap.find(fd) == m_eventMap.end()) {
        LOG_CORE_ERROR("Invalid or unregistered fd = {}", fd);
        return false;
    }

    m_eventMap.erase(fd);
    LOG_CORE_INFO("Removed event for fd = {}", fd);
    return true;
}

void BoostPoller::handleEvent() {
    try {
        m_ioContext.run();
    } catch (const std::exception& e) {
        LOG_CORE_ERROR("Error in BoostPoller::handleEvent: {}", e.what());
    }
}