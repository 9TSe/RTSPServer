#include "MediaSessionManager.h"
#include "MediaSession.h"

MediaSessionManager* MediaSessionManager::Create_New() 
{
    return new MediaSessionManager();
}
MediaSessionManager::MediaSessionManager()
{}

MediaSessionManager::~MediaSessionManager()
{}

bool MediaSessionManager::Add_Session(MediaSession* session) 
{
    if (m_sessionMap.find(session->Name()) != m_sessionMap.end())
    {
        return false;
    }
    else 
    {
        m_sessionMap.insert(std::make_pair(session->Name(), session));
        return true;
    }
}
bool MediaSessionManager::Remove_Session(MediaSession* session) 
{
    std::map<std::string, MediaSession*>::iterator it = m_sessionMap.find(session->Name());
    if (it == m_sessionMap.end()) 
    {
        return false;
    }
    else 
    {
        m_sessionMap.erase(it);
        return true;
    }

}
MediaSession* MediaSessionManager::Get_Session(const std::string& Name) 
{
    std::map<std::string, MediaSession*>::iterator it = m_sessionMap.find(Name);
    if (it == m_sessionMap.end()) 
        return NULL;
    else 
        return it->second;
}