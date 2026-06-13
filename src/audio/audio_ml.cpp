#include "audio_ml.h"

#include <PDM.h>
#include <string.h>

#include <SAR_Device_inferencing.h>
#include <edge-impulse-sdk/dsp/numpy.hpp>

static const float HELP_CANDIDATE_THRESHOLD = 0.75f;
static const float HELP_MARGIN = 0.15f;
static const float HELP_VERY_STRONG_THRESHOLD = 0.97f;
static const float HELP_VERY_STRONG_MARGIN = 0.50f;
static const uint8_t HELP_HISTORY_SIZE = 3;
static const uint8_t HELP_REQUIRED_CANDIDATES = 2;
static const int PDM_GAIN = 127;

static int16_t audioBuffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
static int16_t discardBuffer[256];

static volatile size_t samplesRead = 0;
static volatile bool recording = false;
static volatile bool recordingReady = false;

static float helpScore = 0.0f;
static bool helpDetected = false;
static bool helpCandidateHistory[HELP_HISTORY_SIZE] = {false};
static uint8_t helpHistoryIndex = 0;
static uint8_t helpHistoryCount = 0;

static void clearAudioResult() {
    helpScore = 0.0f;
    helpDetected = false;
}

static void flushPdmBuffer() {
    while (PDM.available() > 0) {
        int bytesToRead = PDM.available();
        if (bytesToRead > (int)sizeof(discardBuffer)) {
            bytesToRead = sizeof(discardBuffer);
        }
        PDM.read(discardBuffer, bytesToRead);
    }
}

static void onPdmData() {
    int bytesAvailable = PDM.available();

    if (bytesAvailable <= 0) {
        return;
    }

    if (!recording || recordingReady) {
        flushPdmBuffer();
        return;
    }

    size_t remainingSamples = EI_CLASSIFIER_RAW_SAMPLE_COUNT - samplesRead;
    int maxBytesToRead = remainingSamples * sizeof(int16_t);

    int bytesToRead = bytesAvailable;
    if (bytesToRead > maxBytesToRead) {
        bytesToRead = maxBytesToRead;
    }

    int bytesRead = PDM.read((uint8_t *)&audioBuffer[samplesRead], bytesToRead);
    int newSamples = bytesRead / sizeof(int16_t);

    samplesRead += newSamples;

    if (samplesRead >= EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        recordingReady = true;
        recording = false;
        flushPdmBuffer();
    }
}

static int getSignalData(size_t offset, size_t length, float *outPtr) {
    numpy::int16_to_float(&audioBuffer[offset], outPtr, length);
    return 0;
}

static bool isHelpLabel(const char *label) {
    return strcmp(label, "AJUTOR") == 0 || strcmp(label, "ajutor") == 0;
}

static bool confirmHelpCandidate(bool candidateHelp) {
    helpCandidateHistory[helpHistoryIndex] = candidateHelp;
    helpHistoryIndex = (helpHistoryIndex + 1) % HELP_HISTORY_SIZE;

    if (helpHistoryCount < HELP_HISTORY_SIZE) {
        helpHistoryCount++;
    }

    uint8_t candidateCount = 0;

    for (uint8_t i = 0; i < helpHistoryCount; i++) {
        if (helpCandidateHistory[i]) {
            candidateCount++;
        }
    }

    return candidateCount >= HELP_REQUIRED_CANDIDATES;
}

static void clearHelpHistory() {
    for (uint8_t i = 0; i < HELP_HISTORY_SIZE; i++) {
        helpCandidateHistory[i] = false;
    }

    helpHistoryIndex = 0;
    helpHistoryCount = 0;
}

static bool runAudioClassifier() {
    clearAudioResult();

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &getSignalData;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

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

    bool veryStrongHelp =
        topIsHelp &&
        helpScore >= HELP_VERY_STRONG_THRESHOLD &&
        margin >= HELP_VERY_STRONG_MARGIN;

    bool candidateHelp =
        topIsHelp &&
        helpScore >= HELP_CANDIDATE_THRESHOLD &&
        margin >= HELP_MARGIN;

    helpDetected =
        veryStrongHelp ||
        confirmHelpCandidate(candidateHelp);

    if (helpDetected) {
        clearHelpHistory();
    }

    return true;
}

bool audio_ml_init() {
    helpScore = 0.0f;
    helpDetected = false;
    clearHelpHistory();
    PDM.onReceive(onPdmData);
    PDM.setBufferSize(4096);
    PDM.setGain(PDM_GAIN);
    return PDM.begin(1, EI_CLASSIFIER_FREQUENCY);

}

bool audio_ml_startCapture() {
    if (recording || recordingReady) {
        return false;
    }

    flushPdmBuffer();

    samplesRead = 0;
    recordingReady = false;
    recording = true;

    return true;
}

bool audio_ml_captureReady() {
    return recordingReady;
}

bool audio_ml_process() {
    if (!recordingReady) {
        return false;
    }

    bool classifierOk = runAudioClassifier();
    recordingReady = false;

    if (!classifierOk) {
        clearAudioResult();
    }

    return classifierOk;
}

float audio_ml_getHelpScore() {
    return helpScore;
}

bool audio_ml_isHelpDetected() {
    return helpDetected;
}
