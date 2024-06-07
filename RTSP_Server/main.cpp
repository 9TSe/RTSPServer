#include "Scheduler/Log.h"
#include "Scheduler/EventScheduler.h"
#include "Scheduler/ThreadPool.h"
#include "Scheduler/UsageEnvironment.h"
#include "Live/InetAddress.h"
#include "Live/MediaSessionManager.h"
#include "Live/RtspServer.h"
#include "Live/MediaSession.h"
#include "Live/H264MediaSource.h"
#include "Live/H264Sink.h"
#include "Live/AACMediaSource.h"
#include "Live/AACSink.h"
int main()
{
	/*
	程序初始化了一份session名为test的资源，访问路径:
	//tcp
	ffplay -i -rtsp_transport tcp  rtsp://127.0.0.1:8554/test

	//udp
	ffplay -i rtsp://127.0.0.1:8554/test
	*/
	srand(time(nullptr));

	EventScheduler* scheduler = EventScheduler::createNew();
	ThreadPool* threadpool = ThreadPool::createNew(1);
	UsageEnvironment* env = UsageEnvironment::createNew(threadpool, scheduler);

	IPV4Address rtspaddr("127.0.0.1", 8554);
	MediaSessionManager* sessionmanager = MediaSessionManager::createNew();
	RtspServer* rtspserver = RtspServer::createNew(env, sessionmanager, rtspaddr);

	LOGI("---session init---");
	MediaSession* mediasession = MediaSession::createNew("test");
	MediaSource* source = H264MediaSource::createNew(env, R"(/home/ninetse/avsource/miku2.h264)");
	Sink* sink = H264Sink::createNew(env, source);
	mediasession->addSink(MediaSession::TRACK_ID0, sink);

	source = AACMediaSource::createNew(env, R"(/home/ninetse/avsource/miku2.aac)");
	sink = AACSink::createNew(env, source);
	mediasession->addSink(MediaSession::TRACK_ID1, sink);


	sessionmanager->addSession(mediasession);
	LOGI("---session end---");

	rtspserver->start();
	env->eventScheduler()->start();
	
	delete scheduler;
	delete threadpool;
	delete env;
	delete sessionmanager;
	delete rtspserver;
	delete mediasession;
	delete source;
	delete sink;
	return 0;
}