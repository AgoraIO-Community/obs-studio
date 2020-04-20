#ifndef mac_agora_rtc_engine_hpp
#define mac_agora_rtc_engine_hpp

#include <stdio.h>
#include <string>

#include <AgoraRtcKit/IAgoraRtcEngine.h>
#include <AgoraRtcKit/IAgoraMediaEngine.h>

using namespace std;
using namespace agora;
using namespace agora::base;
using namespace agora::rtc;
using namespace agora::media;
using namespace agora::util;

enum class ChannelState
{
    Disconnect, Connected
};

class AgoraEngine
{
private:
    IRtcEngine *rtcEngine;
public:
    AgoraEngine(const char* appId, IRtcEngineEventHandler *eventHandle);
    ~AgoraEngine();
    
    ChannelState channelState;
    
    FILE *pfile;

    void enableVideo();
    void joinChannel(const char *id, const char *uid, const char *token, const bool stringified_uid);
    void leaveChannel();
    void enableExAudio(int sampleRate, int channels);
    void pushExAudio(void *rawdata, int samples);
    void enableExVideo(string resolution);
    void pushExVideo(void *rawdata, long long timeStamp, int width, int height);
};

#endif /* mac_agora_rtc_engine_hpp */
