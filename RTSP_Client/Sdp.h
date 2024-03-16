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
	char control[30] = { 0 };     //ʾ��ֵ streamid=0,track=0
	char control_name[30] = { 0 };//��control��ȡ���������ƣ�ʾ��ֵ streamid,track
	int  control_id = -1;		  //ʾ��ֵ TrackNone,TrackH264,TrackAAC ����ö��TrackId

	int playload = -1;   // ����Ƶ����ı��,ʾ��ֵ 96,97

	char codec[20] = { 0 };// �����ʽ
	int timebase = -1;// ʱ���

	int audio_channel = -1;// ��Ƶ����,����
	int audio_as = -1;// ��Ƶ����,AS
	bool is_send_setup = false;//��ǰtrack�Ƿ��Ѿ����͹�setup����
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