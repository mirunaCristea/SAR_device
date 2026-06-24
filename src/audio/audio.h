#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

// Stările posibile ale subsistemului audio după procesarea unei ferestre.
enum AudioState {
    AUDIO_NORMAL,
    AUDIO_HELP_DETECTED,
    AUDIO_UNAVAILABLE
};

// Structură utilizată pentru transmiterea rezultatului detecției audio
// către logica de alertare și către bucla principală.
struct AudioData {
    AudioState state = AUDIO_UNAVAILABLE;

    // Scorul asociat clasei de ajutor, exprimat procentual.
    uint8_t helpScore = 0;

    // Indicatori intermediari folosiți pentru confirmarea detecției.
    bool helpCandidate = false;
    bool helpStrong = false;

    // Indică dacă rezultatul audio curent este utilizabil.
    bool valid = false;
};

// Inițializează subsistemul audio și modelul TinyML.
void audio_init();

// Actualizează rezultatul audio atunci când o fereastră completă a fost procesată.
bool audio_update(AudioData &data);

#endif // AUDIO_H