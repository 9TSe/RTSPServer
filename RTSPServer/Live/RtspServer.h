#pragma once
#include <mutex>
#include <vector>
#include <map>
#include <memory>
#include "InetAddress.h"

class IOEvent;
class TriggerEvent;
class UsageEnvironment;
class MediaSessionManager;
class UsageEnvironment;
class RtspConnection;

class RtspServer : public std::enable_shared_from_this<RtspServer>
{
public:
	using EnvPtr = std::shared_ptr<UsageEnvironment>;
	using ManPtr = std::shared_ptr<MediaSessionManager>;
	RtspServer(EnvPtr env, ManPtr ssmgr, IPV4Address& addr);
	~RtspServer();
	static std::unique_ptr<RtspServer> createNew(EnvPtr env, ManPtr ssmgr, IPV4Address& addr);
	void start();
	UsageEnvironment* getEnv() const { return m_env.get(); }
private:
	static void readCallback(void* arg);
	void handleRead();
	static void disConnectCallbcak(void* arg, int fd);
	void handleDisconnect(int fd);
	static void closeConnectCallback(void* arg);
	void handleCloseConnect();

public:
	ManPtr m_ssmgr;

private:
	EnvPtr m_env;
	IPV4Address m_addr;
	bool m_listen;
	int m_fd;
	IOEvent* m_acceptIOEvent;
	TriggerEvent* m_closeTriggerEvent;
	std::mutex m_mutex;

	std::map<int, RtspConnection*> m_connMap;
	std::vector<int> m_disConnlist;
};