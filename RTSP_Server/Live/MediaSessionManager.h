#pragma once
#include <map>
#include <memory>
#include <string>

class MediaSession;
class MediaSessionManager
{
public:
	using Ptr = std::shared_ptr<MediaSessionManager>;
	static Ptr createNew();
	MediaSessionManager();
	~MediaSessionManager();
	bool addSession(std::shared_ptr<MediaSession> session);

	bool removeSession(MediaSession* session);
	MediaSession* getSession(const std::string& name);

private:
	std::map<std::string, MediaSession*> m_sessions;
};