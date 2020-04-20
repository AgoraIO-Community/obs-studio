//
//  mac-streaming-agora.cpp
//  mac-streaming-agora
//
//  Created by CavanSu on 2019/3/18.
//  Copyright © 2019年 CavanSu. All rights reserved.
//

#pragma clang diagnostic ignored "-Wunused-parameter"

#include <iostream>
#include <stdio.h>
#include <obs-module.h>
#include <obs-avc.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>
#include <fstream>

#include <dlfcn.h>

#include "mac-streaming-agora.hpp"
#include "mac-agora-raw-data.hpp"
#include "mac-agora-encoded-data.hpp"
#include "mac-agora-rtc-engine.hpp"

#include <AgoraRtcKit/IAgoraRtcEngine.h>
#include <AgoraRtcKit/IAgoraMediaEngine.h>

//#include <Quartz/Quartz.h>
//#include <CoreMedia/CoreMedia.h>


using namespace std;
using namespace agora;
using namespace agora::base;
using namespace agora::rtc;
using namespace agora::media;
using namespace agora::util;

#define WriteAudio 0
#define Agora 1
#define ExAudio 1
#define ExVideo 1

static std::unique_ptr<AgoraEngine> agoraKit;

#pragma mark - rtc event handle
class RtcEvent: public IRtcEngineEventHandler
{
public:
    void onJoinChannelSuccess(const char *channel, uid_t uid, int elapsed);
    void onError(int err, const char *msg);
    void onLeaveChannel(const RtcStats &stats);
    void onConnectionLost();
};

void RtcEvent::onJoinChannelSuccess(const char *channel, uid_t uid, int elapsed)
{
    agoraKit->channelState = ChannelState::Connected;
    cout << "AGORA_OBS, onJoinChannelSuccess, channel: " << channel << ", uid: " << uid << endl;
}

void RtcEvent::onError(int err, const char *msg)
{
    if (msg != nullptr)
    {
        cout << "AGORA_OBS, onError, error: " << err << ", msg " << msg << endl;
    }
    else
    {
        cout << "AGORA_OBS, onError, error: " << err << endl;
    }
}

void RtcEvent::onLeaveChannel(const RtcStats &stats)
{
    agoraKit->channelState = ChannelState::Disconnect;
    cout << "AGORA_OBS, onLeaveChannel, stats duration: " << stats.duration << endl;
}

void RtcEvent::onConnectionLost()
{
    agoraKit->channelState = ChannelState::Disconnect;
    cout << "AGORA_OBS, onConnectionLost" << endl;
}

static bool pre_stringified = false;
static std::string pre_uid = "";
static std::string pre_app_id = "";
static std::string pre_app_token = "";
static std::string pre_app_channel = "";
static std::string pre_app_resolution = "";

#pragma mark - rtc engine
void initAgora(const char* appId)
{
    if (agoraKit.get())
    {
        return;
    }
   
    agoraKit.reset(new AgoraEngine(appId, new RtcEvent));
}

#pragma mark - raw data
static const char *agora_rawdata_getname(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "agora_raw_output_info";
}

static void agora_raw_output_update(void *data, obs_data_t *settings) {
    pre_stringified = obs_data_get_bool(settings, "ag_xx_stringified");
    pre_uid = obs_data_get_string(settings, "ag_xx_uid");
    pre_app_id = obs_data_get_string(settings, "ag_app_id");
    pre_app_token = obs_data_get_string(settings, "ag_xx_token");
    pre_app_channel = obs_data_get_string(settings, "ag_xx_channel");
    pre_app_resolution = obs_data_get_string(settings, "ag_xx_resolution");

    initAgora(pre_app_id.c_str());
}

static void *agora_rawdata_create(obs_data_t *settings, obs_output_t *output)
{
    cout << "agora_rawdata_create start " << endl;

    struct agora_raw_output *raw_output = static_cast<agora_raw_output *>(bzalloc(sizeof(struct agora_raw_output)));
    pthread_mutex_init_value(&raw_output->write_mutex);
    raw_output->output = output;

    pthread_mutex_init(&raw_output->write_mutex, NULL);

    UNUSED_PARAMETER(settings);
    cout << "agora_rawdata_create" << endl;
    return raw_output;
}

typedef double (*get_cmtime_t)();

double getTS()
{
    static bool initialized = false;
    static bool valid = false;
    static get_cmtime_t cmtime = nullptr;
    
    if (!initialized)
    {
        void *quartzCore = dlopen("/System/Library/Frameworks/"
                                  "QuartzCore.framework/QuartzCore", RTLD_LAZY);
        if (quartzCore)
        {
            cmtime = (get_cmtime_t)dlsym(quartzCore,
                                                 "CACurrentMediaTime");
            valid = cmtime;
        }
        
        initialized = true;
    }

    if (valid) {
        return cmtime();
    }
    return -1;
}

static bool agora_raw_output_start(void *data)
{
    struct agora_raw_output *raw_output = static_cast<agora_raw_output *>(data);

    cout << "agora_raw_output_start start" << endl;

    if (!obs_output_can_begin_data_capture(raw_output->output, 0))
    {
        return false;
    }
    
    os_atomic_set_bool(&raw_output->stopping, false);
    os_atomic_set_bool(&raw_output->active, true);
    obs_output_begin_data_capture(raw_output->output, 0);

#if WriteAudio
    FILE *pfile;
    pfile = fopen("/Users/admin/Desktop/audioFile/audio_raw.pcm", "wb");
    agoraKit->pfile = pfile;
#endif

#if Agora

#if ExAudio
    agoraKit->enableExAudio(44100, 1);
#endif

#if ExVideo
    agoraKit->enableVideo();
    agoraKit->enableExVideo(pre_app_resolution);
#endif

    agoraKit->joinChannel(pre_app_channel.c_str(), pre_uid.c_str(), pre_app_token.c_str(), pre_stringified);
#endif
    cout << "agora_raw_output_start" << endl;
    return true;
}

static void agora_raw_output_stop(void *data, uint64_t ts)
{
    struct agora_raw_output *raw_output = static_cast<agora_raw_output *>(data);

    cout << "agora_raw_output_stop start" << endl;

    if (!raw_output->active)
    {
        return;
    }

    os_atomic_set_bool(&raw_output->stopping, true);
    obs_output_end_data_capture(raw_output->output);
    os_atomic_set_bool(&raw_output->active, false);
    os_atomic_set_bool(&raw_output->stopping, false);

#if WriteAudio
    fclose(agoraKit->pfile);
#endif
    
#if Agora
    agoraKit->leaveChannel();
#endif
    cout << "agora_raw_output_stop" << endl;
}

static void agora_receive_rawdata_audio(void *data, struct audio_data *frames)
{
    struct agora_raw_output *raw_output = static_cast<agora_raw_output *>(data);
    
    pthread_mutex_lock(&raw_output->write_mutex);

#if WriteAudio
    fwrite(frames->data[0], sizeof(float), frames->frames, agoraKit->pfile);
    pthread_mutex_unlock(&raw_output->write_mutex);
    cout << "write audio~~~~" << endl;
    return;
#else

    int i;
    if (raw_output->stopping || agoraKit->channelState == ChannelState::Disconnect)
    {
        pthread_mutex_unlock(&raw_output->write_mutex);
        return;
    }
#if ExAudio
    int16_t temp[1024];
    float* p = reinterpret_cast<float*>(frames->data[0]);
    for (i = 0; i < 1024; i++) {
        temp[i] = static_cast<int16_t>(p[i] * 32767);
    }
    int16_t *ptemp;
    ptemp = temp;

    agoraKit->pushExAudio(ptemp, frames->frames);
#endif
#endif

    pthread_mutex_unlock(&raw_output->write_mutex);
}

static void agora_receive_rawdata_video(void *data, struct video_data *frame)
{
    struct agora_raw_output *raw_output = static_cast<agora_raw_output *>(data);
    
    pthread_mutex_lock(&raw_output->write_mutex);

    if (raw_output->stopping)
    {
        return;
    }
#if ExVideo
    video_t *video = obs_output_video(raw_output->output);
    const struct video_output_info *videoInfo = video_output_get_info(video);

    int tempSize = videoInfo->width * videoInfo->height * 3 * 0.5;
    unsigned char* temp = (unsigned char*) malloc(tempSize);
    memset(temp, 0, tempSize);

    int ySize = videoInfo->width * videoInfo->height;
    int uvSize = videoInfo->width * videoInfo->height * 0.5;

    // copy Y to temp
    memcpy(temp, frame->data[0], ySize);

    // copy UV to temp
    memcpy(temp + ySize, frame->data[1], uvSize);

    long long tms = (long long) (getTS() * 1000);
    agoraKit->pushExVideo((void*) temp, tms, videoInfo->width, videoInfo->height); // CMTimeMake(CACurrentMediaTime() * 1000, 1000)
    free(temp);
#endif
    pthread_mutex_unlock(&raw_output->write_mutex);
}

static void agora_raw_output_destroy(void *data)
{
    struct agora_raw_output *raw_output = static_cast<agora_raw_output *>(data);

    cout << "agora_raw_output_destroy start" << endl;

    if (!raw_output)
    {
        return;
    }

    if (raw_output->connecting)
    {
        pthread_join(raw_output->start_thread, NULL);
    }

    pthread_mutex_destroy(&raw_output->write_mutex);
    bfree(data);

    agoraKit.release();
    agoraKit.reset();

    cout << "agora_raw_output_destroy" << endl;
}

struct obs_output_info agora_raw_output_info =
{
    .id        = "agora_raw_output_info",
    .flags     = OBS_OUTPUT_AUDIO | OBS_OUTPUT_VIDEO,
    .get_name  = agora_rawdata_getname,
    .create    = agora_rawdata_create,
    .update    = agora_raw_output_update,
    .destroy   = agora_raw_output_destroy,
    .start     = agora_raw_output_start,
    .stop      = agora_raw_output_stop,
    .raw_video = agora_receive_rawdata_video,
    .raw_audio = agora_receive_rawdata_audio,
};

#pragma mark - encoded package
static const char *agora_encoded_getname(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "agora_encoded_output";
}

static void *agora_encoded_create(obs_data_t *settings, obs_output_t *output)
{
    struct agora_encoded_output *encoded_output = static_cast<agora_encoded_output *>(bzalloc(sizeof(struct agora_encoded_output)));
    cout << "agora_create" << endl;
    encoded_output->output = output;
    pthread_mutex_init(&encoded_output->mutex, NULL);

    UNUSED_PARAMETER(settings);
    return encoded_output;
}

static bool agora_encoded_start(void *data)
{
    struct agora_encoded_output *encoded_output = static_cast<agora_encoded_output *>(data);

    if (!obs_output_can_begin_data_capture(encoded_output->output, 0))
    {
        return false;
    }

    if (!obs_output_initialize_encoders(encoded_output->output, 0))
    {
        return false;
    }

    encoded_output->got_first_video = false;
    encoded_output->sent_headers = false;
    os_atomic_set_bool(&encoded_output->stopping, false);

    os_atomic_set_bool(&encoded_output->active, true);
    obs_output_begin_data_capture(encoded_output->output, 0);

    cout << "agora_encoded_start" << endl;
    return true;
}

static void agora_encoded_destroy(void *data)
{
    struct agora_encoded_output *stream = static_cast<agora_encoded_output *>(data);
    
    pthread_mutex_destroy(&stream->mutex);
    cout << "agora_encoded_destroy" << endl;
    bfree(stream);
}

static void agora_encoded_stop(void *data, uint64_t ts)
{
    struct agora_encoded_output *encoded_output = static_cast<agora_encoded_output *>(data);
    os_atomic_set_bool(&encoded_output->stopping, true);
    cout << "agora_stop" << endl;
}

static void agora_encoded_recieve_output_data(void *data, struct encoder_packet *packet)
{
    struct agora_encoded_output *encoded_output = static_cast<agora_encoded_output *>(data);
    if (encoded_output->stopping)
    {
        return;
    }
}

static obs_properties_t *agora_encoded_properties(void *unused)
{
    obs_properties_t *ppts = obs_properties_create();
    obs_properties_add_bool(ppts, "my_bool",
                            "MyBool");
    UNUSED_PARAMETER(unused);
    return ppts;
}

struct obs_output_info agora_encoded_output_info =
{
    .id                   = "agora_encoded_output_info",
    .flags                = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED,
    .encoded_video_codecs = "h264",
    .encoded_audio_codecs = "aac",
    .get_name             = agora_encoded_getname,
    .create               = agora_encoded_create,
    .destroy              = agora_encoded_destroy,
    .start                = agora_encoded_start,
    .stop                 = agora_encoded_stop,
    .encoded_packet       = agora_encoded_recieve_output_data,
    .get_properties       = agora_encoded_properties,
};

/* Defines common functions (required) */
OBS_DECLARE_MODULE()
extern struct obs_output_info agora_encoded_output_info;
extern struct obs_output_info agora_raw_output_info;

bool obs_module_load(void)
{
    blog(LOG_INFO, "agora_loaded");
    obs_register_output(&agora_raw_output_info);
    return true;
}
