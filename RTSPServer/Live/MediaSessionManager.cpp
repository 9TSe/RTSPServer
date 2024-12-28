#include "MediaSessionManager.h"
#include "MediaSession.h"

MediaSessionManager::MediaSessionManager(){}
MediaSessionManager::~MediaSessionManager()
{
	LOG_CORE_INFO("~MediaSessionManager()");
}

std::shared_ptr<MediaSessionManager> MediaSessionManager::createNew()
{
	return std::make_shared<MediaSessionManager>();
}

bool MediaSessionManager::addSession(std::shared_ptr<MediaSession> session)
{
	auto it = m_sessions.find(session->sessionName());
	if (it != m_sessions.end()) return false;
	else
	{
		m_sessions.insert(std::make_pair(session->sessionName(), session.get()));
		return true;
	}
}

bool MediaSessionManager::removeSession(MediaSession* session)
{
	auto it = m_sessions.find(session->sessionName());
	if (it != m_sessions.end())
	{
		m_sessions.erase(it);
		return true;
	}
	else return false;
}

MediaSession* MediaSessionManager::getSession(const std::string& name)
{
	auto it = m_sessions.find(name);
	if (it != m_sessions.end())
		return it->second;
	else
		return nullptr;
}