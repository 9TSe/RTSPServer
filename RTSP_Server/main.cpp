#include "EventScheduler.h"
#include "ThreadPool.h"
#include "UsageEnvironment.h"
#include "MediaSessionManager.h"
#include "RtspServer.h"
#include "H264FileMediaSource.h"
#include "H264FileSink.h"
#include "AACFileMediaSource.h"
#include "AACFileSink.h"
#include "Log.h"

int main() 
{
    /*
    程序初始化了一份session名为test的资源，访问路径:
    // rtp over tcp
    ffplay -i -rtsp_transport tcp  rtsp://127.0.0.1:8554/test

    // rtp over udp
    ffplay -i rtsp://127.0.0.1:8554/test
    */

    srand(time(NULL));//时间初始化

    EventScheduler* scheduler = EventScheduler::Create_New(EventScheduler::POLLER_SELECT);
    ThreadPool* threadpool = ThreadPool::Create_New(1);
    MediaSessionManager* sessmgr = MediaSessionManager::Create_New();
    UsageEnvironment* env = UsageEnvironment::Create_New(scheduler, threadpool);

    Ipv4Address rtspaddr("127.0.0.1", 8554);
    RtspServer* rtspserver = RtspServer::Create_New(env, sessmgr, rtspaddr);

    LOGI("----------session init start------");
    {
        MediaSession* session = MediaSession::Create_New("test");
        MediaSource* source = H264FileMediaSource::Create_New(env, "daliu.h264");
        Sink* sink = H264FileSink::Create_New(env, source);
        session->Add_Sink(MediaSession::TrackId0, sink);

        source = AACFileMediaSource::Create_New(env, "daliu.aac");
        sink = AACFileSink::Create_New(env, source);
        session->Add_Sink(MediaSession::TrackId1, sink);

        //session->startMulticast(); //多播
        sessmgr->Add_Session(session);
    }
    LOGI("----------session init end------");

    rtspserver->Start();
    env->Scheduler()->Loop();
    return 0;
}