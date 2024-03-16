#pragma once
#define TRACK_MAX_NUM 2

struct SdpTrack {
public:
	enum TrackId
	{
		TrackNone = -1,
		TrackH264 = 0,
		TrackAAC = 1
	};
	SdpTrack();
	~SdpTrack();
	SdpTrack(const SdpTrack& track);

public:
	bool isAlive = false;
	char control[30] = { 0 };     //示例值 streamid=0,track=0
	char control_name[30] = { 0 };//从control提取出来的名称，示例值 streamid,track
	int  control_id = -1;		  //示例值 TrackNone,TrackH264,TrackAAC 来自枚举TrackId

	int playload = -1;   // 音视频编码的编号,示例值 96,97

	char codec[20] = { 0 };// 编码格式
	int timebase = -1;// 时间基

	int audio_channel = -1;// 音频特有,声道
	int audio_as = -1;// 音频特有,AS
	bool is_send_setup = false;//当前track是否已经发送过setup请求
};

class Sdp
{
public:
	explicit Sdp();
	~Sdp();
public:
	int Parse(char* buf, size_t size);
	SdpTrack* Pop_Track();
	SdpTrack tracks[TRACK_MAX_NUM];
};