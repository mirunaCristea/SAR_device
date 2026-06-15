#include "audio.h"
#include "audio_ml.h"

#include <Arduino.h>
#include <string.h>

// ===================== CONFIG DETECTOR AUDIO =====================
//
// audio_ml.cpp clasifica fiecare fereastra audio:
// - helpCandidate: fereastra seamana moderat cu "ajutor"
// - helpStrong: fereastra este foarte sigura
//
// audio.cpp face decizia temporala finala:
// alerta daca:
// 1) exista cel putin o fereastra STRONG
// SAU
// 2) minimum 3 ferestre CANDIDATE in ultimele 12 ferestre
//    si raportul candidate este >= 25%
//
// Cu overlap de 500 ms, 12 ferestre inseamna aproximativ 6 s.

static const uint8_t AUDIO_DETECTOR_WINDOW_SIZE = 12;
static const uint8_t AUDIO_DETECTOR_MIN_WINDOWS = 6;
static const uint8_t AUDIO_DETECTOR_MIN_CANDIDATES = 3;
static const uint8_t AUDIO_DETECTOR_MIN_STRONG = 1;
static const float AUDIO_DETECTOR_CANDIDATE_RATIO = 0.25f;

static const unsigned long AUDIO_ALERT_SUPPRESSION_MS = 7000;

// ===================== STATE AUDIO =====================

static bool audioReady = false;
static bool captureActive = false;

// ===================== STATE DETECTOR TEMPORAL =====================

struct AudioDetectorStatus {
    bool alertNow = false;
    bool rawAlert = false;

    uint8_t candidateCount = 0;
    uint8_t strongCount = 0;
    uint8_t totalCount = 0;

    float candidateRatio = 0.0f;
};

static bool candidateBuffer[AUDIO_DETECTOR_WINDOW_SIZE];
static bool strongBuffer[AUDIO_DETECTOR_WINDOW_SIZE];

static uint8_t detectorIndex = 0;
static uint8_t detectorCount = 0;

static unsigned long lastAudioAlertTime = 0;

// ===================== FUNCTII AUXILIARE =====================

static void clearDetectorBuffers()
{
    for (uint8_t i = 0; i < AUDIO_DETECTOR_WINDOW_SIZE; i++) {
        candidateBuffer[i] = false;
        strongBuffer[i] = false;
    }

    detectorIndex = 0;
    detectorCount = 0;
}

void audio_reset_detector()
{
    clearDetectorBuffers();
}

static void setUnavailable(AudioData &data)
{
    data.state = AUDIO_UNAVAILABLE;

    data.helpScore = 0;

    data.helpCandidate = false;
    data.helpStrong = false;

    data.helpAlertRaw = false;
    data.helpAlertNow = false;

    data.detectorCandidateCount = 0;
    data.detectorStrongCount = 0;
    data.detectorWindowCount = 0;
    data.detectorCandidateRatioPercent = 0;

    data.valid = false;
}

static void setDetectorFields(
    AudioData &data,
    const AudioDetectorStatus &status
)
{
    data.helpAlertRaw = status.rawAlert;
    data.helpAlertNow = status.alertNow;

    data.detectorCandidateCount = status.candidateCount;
    data.detectorStrongCount = status.strongCount;
    data.detectorWindowCount = status.totalCount;

    data.detectorCandidateRatioPercent =
        static_cast<uint8_t>(status.candidateRatio * 100.0f + 0.5f);
}

static AudioDetectorStatus updateDetector(
    bool candidateHelp,
    bool strongHelp
)
{
    AudioDetectorStatus status;

    candidateBuffer[detectorIndex] = candidateHelp;
    strongBuffer[detectorIndex] = strongHelp;

    detectorIndex =
        (detectorIndex + 1) % AUDIO_DETECTOR_WINDOW_SIZE;

    if (detectorCount < AUDIO_DETECTOR_WINDOW_SIZE) {
        detectorCount++;
    }

    for (uint8_t i = 0; i < detectorCount; i++) {
        if (candidateBuffer[i]) {
            status.candidateCount++;
        }

        if (strongBuffer[i]) {
            status.strongCount++;
        }
    }

    status.totalCount = detectorCount;

    if (status.totalCount > 0) {
        status.candidateRatio =
            status.candidateCount / (float)status.totalCount;
    }

    bool detectedByStrong =
        status.strongCount >= AUDIO_DETECTOR_MIN_STRONG;

    bool detectedByRatio =
        status.totalCount >= AUDIO_DETECTOR_MIN_WINDOWS &&
        status.candidateCount >= AUDIO_DETECTOR_MIN_CANDIDATES &&
        status.candidateRatio >= AUDIO_DETECTOR_CANDIDATE_RATIO;

    status.rawAlert =
        detectedByStrong || detectedByRatio;

    unsigned long now = millis();

    bool suppressionActive =
        lastAudioAlertTime > 0 &&
        now - lastAudioAlertTime < AUDIO_ALERT_SUPPRESSION_MS;

    if (status.rawAlert && !suppressionActive) {
        status.alertNow = true;
        lastAudioAlertTime = now;
    }

    return status;
}

// ===================== API AUDIO =====================

void audio_init()
{
    audioReady = audio_ml_init();
    captureActive = false;

    clearDetectorBuffers();
    lastAudioAlertTime = 0;

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

    data.helpScore =
        static_cast<uint8_t>(score * 100.0f + 0.5f);

    data.helpCandidate = audio_ml_isHelpCandidate();
    data.helpStrong = audio_ml_isHelpStrong();

    AudioDetectorStatus detectorStatus =
        updateDetector(data.helpCandidate, data.helpStrong);

    setDetectorFields(data, detectorStatus);

    data.state =
        detectorStatus.alertNow ? AUDIO_HELP_DETECTED : AUDIO_NORMAL;

    data.valid = true;

    return true;
}