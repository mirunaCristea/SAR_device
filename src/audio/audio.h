#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>


enum AudioState {
    AUDIO_NORMAL,
    AUDIO_HELP_DETECTED,
    AUDIO_UNAVAILABLE
};

struct AudioData {
    AudioState state = AUDIO_UNAVAILABLE;
    uint8_t helpScore = 0;
    bool valid = false;
};

void audio_init();
bool audio_update(AudioData &data);

#endif
