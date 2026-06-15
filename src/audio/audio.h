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

    // Rezultat pe fereastra curenta
    bool helpCandidate = false;
    bool helpStrong = false;

    // Rezultat detector temporal
    bool helpAlertRaw = false;
    bool helpAlertNow = false;

    uint8_t detectorCandidateCount = 0;
    uint8_t detectorStrongCount = 0;
    uint8_t detectorWindowCount = 0;
    uint8_t detectorCandidateRatioPercent = 0;

    bool valid = false;
};

void audio_init();
bool audio_update(AudioData &data);

// Folosit cand audio nu trebuie analizat temporar,
// de exemplu in fereastra critica IMU.
void audio_reset_detector();

#endif