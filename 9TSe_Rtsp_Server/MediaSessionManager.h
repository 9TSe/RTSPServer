#pragma once
#include <map>
#include <string>
class MediaSession;
class MediaSessionManager
{
public:
	static MediaSessionManager* Create_New();
	MediaSessionManager();
	~MediaSessionManager();
	
	bool Add_Session(MediaSession* session);
	bool Remove_Session(MediaSession* session);
	MediaSession* Get_Session(const std::string& name);

private:
	std::map<std::string, MediaSession*> m_sessionMap;
};