
#include "mac-agora-rtc-engine.hpp"

#include <util/platform.h>

#include <AgoraRtcKit/IAgoraRtcEngine.h>
#include <AgoraRtcKit/IAgoraMediaEngine.h>

#pragma mark - file private
IRtcEngine *createRtcEngine(const char *appId, IRtcEngineEventHandler *eventHandle)
{
    RtcEngineContext engineContext;
    engineContext.appId = appId;
    engineContext.eventHandler = eventHandle;
    IRtcEngine *rtcEngine = createAgoraRtcEngine();
    rtcEngine->initialize(engineContext);
    // TODO enable setAppType

    rtcEngine->setChannelProfile(CHANNEL_PROFILE_LIVE_BROADCASTING);
    rtcEngine->enableAudio();

    agora::rtc::RtcEngineParameters ap(*rtcEngine);
    ap.enableDualStreamMode(true);

    return rtcEngine;
}

#pragma mark - public
AgoraEngine::AgoraEngine(const char* appId, IRtcEngineEventHandler *eventHandle)
{
    this->rtcEngine = createRtcEngine(appId, eventHandle);
    this->channelState = ChannelState::Disconnect;
    agora::rtc::RtcEngineParameters aap(rtcEngine);
    char *path = os_get_agora_log_path_ptr("agora_rtc_obs.log");
    aap.setLogFile(path);
}

AgoraEngine::~AgoraEngine()
{
    rtcEngine->release();
}

void AgoraEngine::enableVideo()
{
    rtcEngine->enableVideo();
}

void AgoraEngine::joinChannel(const char *id, const char *uid, const char *token, const bool stringified_uid)
{
    rtcEngine->setClientRole(CLIENT_ROLE_BROADCASTER);

    // TODO Need to check if uid/channel valid
    if (stringified_uid) {
        rtcEngine->joinChannelWithUserAccount(token, id, uid);
    } else {
        uint32_t int_uid = static_cast<uint32_t>(std::stoul(uid));
        rtcEngine->joinChannel(token, id, nullptr, int_uid);
    }
}

void AgoraEngine::leaveChannel()
{
    rtcEngine->leaveChannel();
}

void AgoraEngine::enableExAudio(int sampleRate, int channels)
{
    int perCall = sampleRate * channels / 100;
    RAW_AUDIO_FRAME_OP_MODE_TYPE type = RAW_AUDIO_FRAME_OP_MODE_READ_WRITE;

    agora::rtc::AParameter ap(rtcEngine);
    ap->setParameters("{\"che.audio.external_capture\":true}");
    ap->setParameters("{\"che.audio.external_capture.push\":true}");

    agora::rtc::RtcEngineParameters aap(rtcEngine);
    aap.setRecordingAudioFrameParameters(sampleRate, channels, type, perCall);

    ap->setParameters("{\"che.audio.keep.audiosession\":true}");
}

void AgoraEngine::pushExAudio(void *rawdata, int samples)
{
    IAudioFrameObserver::AudioFrame frame;
    frame.buffer = rawdata;
    frame.samples = samples;
    
    MEDIA_SOURCE_TYPE type;
    type = AUDIO_RECORDING_SOURCE;
    
    agora::util::AutoPtr<agora::media::IMediaEngine> media;
    media.queryInterface(rtcEngine, AGORA_IID_MEDIA_ENGINE);
    media->pushAudioFrame(type, &frame, true);
}

void AgoraEngine::enableExVideo(std::string resolution)
{
    agora::util::AutoPtr<agora::media::IMediaEngine> media;
    media.queryInterface(rtcEngine, AGORA_IID_MEDIA_ENGINE);
    media->setExternalVideoSource(true, false);

    agora::rtc::VideoEncoderConfiguration configuration;
    
    if (resolution.find("360") != string::npos)
    {
        configuration.dimensions.width = 640;
        configuration.dimensions.height = 360;
        configuration.frameRate = (agora::rtc::FRAME_RATE) 15;
    }
    else if (resolution.find("480") != string::npos)
    {
        configuration.dimensions.width = 840;
        configuration.dimensions.height = 480;
        configuration.frameRate = (agora::rtc::FRAME_RATE) 24;
    }
    else if (resolution.find("720") != string::npos)
    {
        configuration.dimensions.width = 1280;
        configuration.dimensions.height = 720;
        configuration.frameRate = (agora::rtc::FRAME_RATE) 24;
    }
    else if (!(resolution.find("1080") != string::npos))
    {
        configuration.dimensions.width = 1920;
        configuration.dimensions.height = 1080;
        configuration.frameRate = (agora::rtc::FRAME_RATE) 30;
    }
    
    configuration.orientationMode = ORIENTATION_MODE_FIXED_LANDSCAPE;

    rtcEngine->setVideoEncoderConfiguration(configuration);

    agora::rtc::RtcEngineParameters ap(*rtcEngine);
    ap.enableWebSdkInteroperability(true);
}

void AgoraEngine::pushExVideo(void *rawdata, long long timeStamp, int width, int height)
{
    ExternalVideoFrame frame;
    frame.stride = width;
    frame.height = height;
    frame.rotation = 0;
    frame.timestamp = timeStamp;
    frame.type = ExternalVideoFrame::VIDEO_BUFFER_TYPE::VIDEO_BUFFER_RAW_DATA;
    frame.format = ExternalVideoFrame::VIDEO_PIXEL_FORMAT::VIDEO_PIXEL_NV12;
    frame.cropTop = 0;
    frame.cropLeft = 0;
    frame.cropRight = 0;
    frame.cropBottom = 0;
    frame.buffer = rawdata;

    agora::util::AutoPtr<agora::media::IMediaEngine> media;
    media.queryInterface(rtcEngine, AGORA_IID_MEDIA_ENGINE);
    media->pushVideoFrame(&frame);
}
