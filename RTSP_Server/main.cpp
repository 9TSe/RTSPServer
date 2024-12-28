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
	//tcp
	ffplay -i -rtsp_transport tcp  rtsp://127.0.0.1:8554/test
	//udp
	ffplay -i rtsp://127.0.0.1:8554/test
	*/
	spdLog::Log::Init();
	srand(time(nullptr));

	auto scheduler = EventScheduler::createNew();
	auto threadpool = ThreadPool::createNew(2);
	auto env = UsageEnvironment::createNew(std::move(threadpool), std::move(scheduler));

	IPV4Address rtspaddr("127.0.0.1", 8554);
	auto sessionmanager = MediaSessionManager::createNew();
	auto rtspserver = RtspServer::createNew(env, sessionmanager, rtspaddr);

	LOG_CORE_INFO("---session init---");
	auto mediasession = MediaSession::createNew("test");
	MediaSource* source = H264MediaSource::createNew(env, R"(/home/ninetse/avsource/miku2.h264)");
	Sink* sink = H264Sink::createNew(env, source);
	mediasession->addSink(MediaSession::TRACK_ID0, sink);

	source = AACMediaSource::createNew(env, R"(/home/ninetse/avsource/miku2.aac)");
	sink = AACSink::createNew(env, source);
	mediasession->addSink(MediaSession::TRACK_ID1, sink);

	// mediasession->startMulticast(); //多播
	sessionmanager->addSession(mediasession);
	LOG_CORE_INFO("---session end---");

	rtspserver->start();
	env->eventScheduler()->start();
	
	delete source;
	delete sink;
	return 0;
}