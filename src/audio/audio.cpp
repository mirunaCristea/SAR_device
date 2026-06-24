#include "audio.h"
#include "audio_ml.h"

#include <Arduino.h>

// Starea internă a subsistemului audio.
static bool audioReady = false;
static bool captureActive = false;

static void setUnavailable(AudioData &data)
{
    data.state = AUDIO_UNAVAILABLE;
    data.helpScore = 0;
    data.helpCandidate = false;
    data.helpStrong = false;
    data.valid = false;
}

void audio_init()
{
    // Inițializează lanțul TinyML audio: microfon PDM, buffer și model.
    audioReady = audio_ml_init();

    Serial.println(audioReady ? "Audio ML OK" : "Audio ML unavailable");
}

bool audio_update(AudioData &data)
{
    if (!audioReady) {
        setUnavailable(data);
        return false;
    }

    // Dacă nu există deja o captură activă, se pornește achiziția unei ferestre audio.
    if (!captureActive) {
        captureActive = audio_ml_startCapture();
        return false;
    }

    // Funcția revine fără procesare până când fereastra audio este completă.
    if (!audio_ml_captureReady()) {
        return false;
    }

    captureActive = false;

    // După captarea ferestrei, se rulează inferența TinyML.
    if (!audio_ml_process()) {
        setUnavailable(data);
        return true;
    }

    float score = constrain(audio_ml_getHelpScore(), 0.0f, 1.0f);

    data.helpScore =
        static_cast<uint8_t>(score * 100.0f + 0.5f);

    // Indicatori utilizați pentru diferențierea între o detecție posibilă și o detecție puternică a apelului de ajutor.
    data.helpCandidate = audio_ml_isHelpCandidate();
    data.helpStrong = audio_ml_isHelpStrong();

    // Starea audio este determinată de indicatorul de detecție puternică.
    data.state =
        data.helpStrong ? AUDIO_HELP_DETECTED : AUDIO_NORMAL;

    data.valid = true;

    return true;
}
