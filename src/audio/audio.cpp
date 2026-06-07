#include "audio.h"
#include "audio_ml.h"

#include <Arduino.h>

static bool audioReady = false;
static bool captureActive = false;

static void setUnavailable(AudioData &data)
{
    data.state = AUDIO_UNAVAILABLE;
    data.helpScore = 0;
    data.valid = false;
}

void audio_init()
{
    audioReady = audio_ml_init();
    Serial.println(audioReady ? "Audio ML OK" : "Audio ML unavailable");
}

bool audio_update(AudioData &data)
{
    if (!audioReady) {
        setUnavailable(data);
        return false;
    }

    if (!captureActive) {
        captureActive = audio_ml_startCapture();
        return false;
    }

    if (!audio_ml_captureReady()) {
        return false;
    }

    captureActive = false;

    if (!audio_ml_process()) {
        setUnavailable(data);
        return true;
    }

    float score = constrain(audio_ml_getHelpScore(), 0.0f, 1.0f);

    data.state =
        audio_ml_isHelpDetected() ? AUDIO_HELP_DETECTED : AUDIO_NORMAL;
    data.helpScore = static_cast<uint8_t>(score * 100.0f + 0.5f);
    data.valid = true;

    return true;
}
