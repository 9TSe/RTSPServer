#pragma once
#include <mutex>
#include <vector>
#include <map>
#include "InetAddress.h"


class IOEvent;
class TriggerEvent;
class UsageEnvironment;
class MediaSessionManager;
class UsageEnvironment;
class RtspConnection;

class RtspServer
{
public:
	RtspServer(UsageEnvironment* env, MediaSessionManager* ssmgr, IPV4Address& addr);
	~RtspServer();
	static RtspServer* createNew(UsageEnvironment* env, MediaSessionManager* ssmgr, IPV4Address& addr);
	void start();
	UsageEnvironment* getEnv() const { return m_env; }
private:
	static void readCallback(void* arg);
	void handleRead();
	static void disConnectCallbcak(void* arg, int fd);
	void handleDisconnect(int fd);
	static void closeConnectCallback(void* arg);
	void handleCloseConnect();

public:
	MediaSessionManager* m_ssmgr;

private:
	UsageEnvironment* m_env;
	IPV4Address m_addr;
	bool m_listen;
	int m_fd;
	IOEvent* m_acceptIOEvent;
	TriggerEvent* m_closeTriggerEvent;
	std::mutex m_mutex;

	std::map<int, RtspConnection*> m_connMap;
	std::vector<int> m_disConnlist;
};