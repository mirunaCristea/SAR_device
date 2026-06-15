#include "audio_ml.h"

#include <PDM.h>
#include <string.h>

#include <SAR_Device_inferencing.h>
#include <edge-impulse-sdk/dsp/numpy.hpp>

// ===================== CONFIG DECIZIE PE FEREASTRA =====================

// Fereastra candidata: intra in calculul procentului pe episod
static const float HELP_CANDIDATE_THRESHOLD = 0.53f;
static const float HELP_CANDIDATE_MARGIN = 0.08f;

// Fereastra foarte sigura: poate valida episodul direct
static const float HELP_STRONG_THRESHOLD = 0.95f;
static const float HELP_STRONG_MARGIN = 0.70f;

static const int PDM_GAIN = 127;

// Overlap intre ferestre
static const uint16_t AUDIO_OVERLAP_MS = 500;

static const size_t AUDIO_OVERLAP_SAMPLES =
    (EI_CLASSIFIER_FREQUENCY * AUDIO_OVERLAP_MS) / 1000;

static const bool DEBUG_AUDIO_SCORES = true;

// ===================== BUFFERE =====================

static int16_t audioBuffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
static int16_t discardBuffer[256];

static volatile size_t samplesRead = 0;
static volatile bool recording = false;
static volatile bool recordingReady = false;

// ===================== REZULTAT =====================

static float helpScore = 0.0f;
static bool helpCandidate = false;
static bool helpStrong = false;

// ===================== AUXILIARE =====================

static void clearAudioResult()
{
    helpScore = 0.0f;
    helpCandidate = false;
    helpStrong = false;
}

static void flushPdmBuffer()
{
    while (PDM.available() > 0) {
        int bytesToRead = PDM.available();

        if (bytesToRead > (int)sizeof(discardBuffer)) {
            bytesToRead = sizeof(discardBuffer);
        }

        PDM.read(discardBuffer, bytesToRead);
    }
}

static void onPdmData()
{
    int bytesAvailable = PDM.available();

    if (bytesAvailable <= 0) {
        return;
    }

    if (!recording || recordingReady) {
        flushPdmBuffer();
        return;
    }

    size_t remainingSamples =
        EI_CLASSIFIER_RAW_SAMPLE_COUNT - samplesRead;

    int maxBytesToRead =
        remainingSamples * sizeof(int16_t);

    int bytesToRead = bytesAvailable;

    if (bytesToRead > maxBytesToRead) {
        bytesToRead = maxBytesToRead;
    }

    int bytesRead =
        PDM.read((uint8_t *)&audioBuffer[samplesRead], bytesToRead);

    int newSamples = bytesRead / sizeof(int16_t);

    samplesRead += newSamples;

    if (samplesRead >= EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        recordingReady = true;
        recording = false;
        flushPdmBuffer();
    }
}

static int getSignalData(size_t offset, size_t length, float *outPtr)
{
    numpy::int16_to_float(&audioBuffer[offset], outPtr, length);
    return 0;
}

static bool isHelpLabel(const char *label)
{
    return strcmp(label, "AJUTOR") == 0 ||
           strcmp(label, "Ajutor") == 0 ||
           strcmp(label, "ajutor") == 0 ||
           strcmp(label, "HELP") == 0 ||
           strcmp(label, "help") == 0;
}

// ===================== CLASIFICARE =====================

static bool runAudioClassifier()
{
    clearAudioResult();

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &getSignalData;

    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR res =
        run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) {
        return false;
    }

    float bestOtherScore = 0.0f;

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        const char *label = result.classification[i].label;
        float value = result.classification[i].value;

        if (isHelpLabel(label)) {
            helpScore = value;
        } else if (value > bestOtherScore) {
            bestOtherScore = value;
        }
    }

    float margin = helpScore - bestOtherScore;
    bool topIsHelp = helpScore > bestOtherScore;

    helpCandidate =
        topIsHelp &&
        helpScore >= HELP_CANDIDATE_THRESHOLD &&
        margin >= HELP_CANDIDATE_MARGIN;

    helpStrong =
        topIsHelp &&
        helpScore >= HELP_STRONG_THRESHOLD &&
        margin >= HELP_STRONG_MARGIN;

    if (DEBUG_AUDIO_SCORES) {
        Serial.print("HELP score=");
        Serial.print(helpScore, 3);

        Serial.print(" | bestOther=");
        Serial.print(bestOtherScore, 3);

        Serial.print(" | margin=");
        Serial.print(margin, 3);

        Serial.print(" | topIsHelp=");
        Serial.print(topIsHelp ? "YES" : "NO");

        Serial.print(" | candidate=");
        Serial.print(helpCandidate ? "YES" : "NO");

        Serial.print(" | strong=");
        Serial.println(helpStrong ? "YES" : "NO");
    }

    return true;
}

// ===================== INIT =====================

bool audio_ml_init()
{
    clearAudioResult();

    samplesRead = 0;
    recording = false;
    recordingReady = false;

    PDM.onReceive(onPdmData);
    PDM.setBufferSize(4096);
    PDM.setGain(PDM_GAIN);

    return PDM.begin(1, EI_CLASSIFIER_FREQUENCY);
}

// ===================== OVERLAP =====================

static void prepareNextOverlappedWindow(bool keepOverlap)
{
    recording = false;
    recordingReady = false;

    if (!keepOverlap ||
        AUDIO_OVERLAP_SAMPLES == 0 ||
        AUDIO_OVERLAP_SAMPLES >= EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        samplesRead = 0;
        return;
    }

    size_t startIndex =
        EI_CLASSIFIER_RAW_SAMPLE_COUNT - AUDIO_OVERLAP_SAMPLES;

    memmove(
        audioBuffer,
        &audioBuffer[startIndex],
        AUDIO_OVERLAP_SAMPLES * sizeof(int16_t)
    );

    samplesRead = AUDIO_OVERLAP_SAMPLES;
}

// ===================== CAPTURA =====================

bool audio_ml_startCapture()
{
    if (recording || recordingReady) {
        return false;
    }

    flushPdmBuffer();

    if (samplesRead >= EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        samplesRead = 0;
    }

    recordingReady = false;
    recording = true;

    return true;
}

bool audio_ml_captureReady()
{
    return recordingReady;
}

bool audio_ml_process()
{
    if (!recordingReady) {
        return false;
    }

    bool classifierOk = runAudioClassifier();

    if (!classifierOk) {
        clearAudioResult();
        prepareNextOverlappedWindow(false);
        return false;
    }

    prepareNextOverlappedWindow(true);
    return true;
}

// ===================== GETTERE =====================

float audio_ml_getHelpScore()
{
    return helpScore;
}

bool audio_ml_isHelpCandidate()
{
    return helpCandidate;
}

bool audio_ml_isHelpStrong()
{
    return helpStrong;
}