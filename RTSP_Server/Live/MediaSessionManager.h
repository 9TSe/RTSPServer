#pragma once
#include <map>
#include <string>

class MediaSession;
class MediaSessionManager
{
public:
	MediaSessionManager();
	~MediaSessionManager();
	static MediaSessionManager* createNew();

	bool addSession(MediaSession* session);
	bool removeSession(MediaSession* session);
	MediaSession* getSession(const std::string& name);

private:
	std::map<std::string, MediaSession*> m_sessions;
};