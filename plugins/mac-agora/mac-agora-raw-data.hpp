#ifndef mac_agora_raw_data_hpp
#define mac_agora_raw_data_hpp

#include <stdio.h>
#include <obs-module.h>

struct agora_raw_output {
    obs_output_t       *output;
    volatile bool      active;
    volatile bool      stopping;
    bool               connecting;
    bool               write_thread_active;
    pthread_t          start_thread;
    pthread_mutex_t    write_mutex;
    pthread_t          write_thread;
};

#endif /* mac_agora_raw_data_hpp */
