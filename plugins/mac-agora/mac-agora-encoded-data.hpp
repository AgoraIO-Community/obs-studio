#ifndef mac_agora_encoded_data_hpp
#define mac_agora_encoded_data_hpp

#include <stdio.h>
#include <obs-module.h>

struct agora_encoded_output
{
    obs_output_t    *output;
    volatile bool   active;
    volatile bool   stopping;
    bool            sent_headers;
    bool            got_first_video;
    uint64_t        stop_ts;
    int32_t         start_dts_offset;
    int64_t         last_packet_ts;
    pthread_mutex_t mutex;
};

#endif /* mac_agora_encoded_data_hpp */
