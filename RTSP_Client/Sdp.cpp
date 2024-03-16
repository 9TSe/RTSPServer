#include "Sdp.h"
#include <map>
#include <vector>
#include <string>
#include "Log.h"

SdpTrack::SdpTrack() {}
SdpTrack::~SdpTrack() {}
SdpTrack::SdpTrack(const SdpTrack& track) 
{
	this->isAlive = track.isAlive;
	memcpy(this->control, track.control, strlen(track.control));
	memcpy(this->control_name, track.control_name, strlen(track.control_name));
	this->control_id = track.control_id;
	this->playload = track.playload;

	memcpy(this->codec, track.codec, strlen(track.codec));
	this->timebase = track.timebase;
	this->audio_channel = track.audio_channel;
	this->audio_as = track.audio_as;

	this->is_send_setup = track.is_send_setup;
}

Sdp::Sdp() {}
Sdp::~Sdp() {}

std::vector<std::string> stringSplit(const std::string& str, const std::string& sep) 
{
	std::vector<std::string> sv;
	int sepSize = sep.size();

	int lastPosition = 0, index = -1;
	while ((index = str.find(sep, lastPosition)) != - 1)
	{
		sv.push_back(str.substr(lastPosition, index - lastPosition));
		lastPosition = index + sepSize;
	}
	std::string lastString = str.substr(lastPosition);//截取最后一个分隔符后的内容

	if (!lastString.empty()) 
		sv.push_back(lastString);//如果最后一个分隔符后还有内容就入队

	return sv;
}

int Sdp::Parse(char* buf, size_t size) 
{
	std::map<std::string, SdpTrack> tmpTrackMap;
	std::string currentMediaName;
	bool currentParseMedia = false;

	const char* sep = "\n";
	char* line = strtok(buf, sep);
	while (line) 
	{
		if (strstr(line, "m=")) // m=video 0 RTP/AVP 96 m=audio 0 RTP/AVP 97
		{ 
			char sdp_m_name[10] = { 0 };
			int  sdp_m_seq = 0;
			char sdp_m_transport[10] = { 0 };
			int  sdp_m_playload = 0;

			if (sscanf(line, "m=%s %d %s %d\r\n", &sdp_m_name, &sdp_m_seq, &sdp_m_transport, &sdp_m_playload) != 4)
				LOGE("parse sdp m error");

			if (!strcmp(sdp_m_name, "video"))
				currentMediaName = sdp_m_name;

			else if (!strcmp(sdp_m_name, "audio")) 
				currentMediaName = sdp_m_name;

			else 
			{
				LOGE("parse sdp m name error");
				break;
			}
			SdpTrack* track = &(tmpTrackMap[currentMediaName]);

			track->isAlive = true;
			track->playload = sdp_m_playload;
			currentParseMedia = true;

		}
		else if (currentParseMedia) 
		{
			SdpTrack* track = &(tmpTrackMap[currentMediaName]);

			if (strstr(line, "a=control:")) 
			{
				char trackNameAndId[50] = { 0 };
				if (sscanf(line, "a=control:%s\r\n", &trackNameAndId) != 1) 
					LOGE("parse sdp a=control error");
				else 
				{
					//赋值control
					memcpy(track->control, trackNameAndId, strlen(trackNameAndId));

					std::vector<std::string> arr = stringSplit(trackNameAndId, "=");
					if (arr.size() == 2) 
					{
						//赋值control_name
						memcpy(track->control_name, arr[0].data(), strlen(arr[0].data()));
						//赋值control_id
						track->control_id = atoi(arr[1].data());
					}
					else 
						LOGE("parse sdp a=control val error");
				}
			}
			else if (strstr(line, "a=rtpmap:")) 
			{
				// a=rtpmap:96 H264/90000
				// a=rtpmap:97 MPEG4-GENERIC/44100/2
				char val[30] = { 0 };
				if (96 == track->playload) 
				{
					if (sscanf(line, "a=rtpmap:96 %s\r\n", &val) != 1) 
						LOGE("parse sdp a=rtpmap:96 error");
					else 
					{
						std::vector<std::string> arr = stringSplit(val, "/");
						if (arr.size() == 2) 
						{
							memcpy(track->codec, arr[0].data(), strlen(arr[0].data()));// 视频编码
							track->timebase = atoi(arr[1].data());// 视频编码时间基

						}
						else 
							LOGE("parse sdp a=rtpmap:96 val error");
					}
				}
				else if (97 == track->playload) 
				{

					if (sscanf(line, "a=rtpmap:97 %s\r\n", &val) != 1) 
						LOGE("parse sdp a=rtpmap:97 error");
					else 
					{
						std::vector<std::string> arr = stringSplit(val, "/");
						if (arr.size() == 3) 
						{
							memcpy(track->codec, arr[0].data(), strlen(arr[0].data()));// 音频编码
							track->timebase = atoi(arr[1].data());// 音频频率
							track->audio_channel = atoi(arr[2].data());// 音频特有 声道
						}
						else 
							LOGE("parse sdp a=rtpmap:97 val error");
					}
				}
				else 
					LOGE("parse sdp a=rtpmap error");
			}
			else if (strstr(line, "a=fmtp:")) {}
			else if (strstr(line, "b=")) 
			{ // 音频特有
				int audio_as = 0;
				if (sscanf(line, "b=AS:%d\r\n", &audio_as) != 1) 
					LOGE("parse sdp b=AS error");
				else 
					track->audio_as = audio_as;
			}
		}
		line = strtok(NULL, sep);
	}

	for (auto& tmp : tmpTrackMap) 
	{
		SdpTrack tmp_track = tmp.second;
		if (tmp_track.control_id < TRACK_MAX_NUM) 
		{
			SdpTrack track(tmp_track);
			tracks[track.control_id] = track;
		}
	}
	return 0;
}

SdpTrack* Sdp::Pop_Track() {

	for (int index = 0; index < TRACK_MAX_NUM; index++)
	{
		if (tracks[index].isAlive && !tracks[index].is_send_setup) 
		{
			tracks[index].is_send_setup = true;
			return &tracks[index];
		}
	}
	return nullptr;
}
