#include "audio_ml.h"

#include <Arduino.h>
#include <PDM.h>
#include <string.h>

#include <SAR_Device_inferencing.h>
#include <edge-impulse-sdk/dsp/numpy.hpp>

static const float HELP_CANDIDATE_THRESHOLD = 0.82f;
static const float HELP_STRONG_THRESHOLD = 0.92f;
static const float HELP_MARGIN = 0.15f;
static const float HELP_STRONG_MARGIN = 0.22f;
static const uint8_t HELP_CONFIRM_COUNT = 2;

static int16_t audioBuffer[EI_CLASSIFIER_RAW_SAMPLE_COUNT];
static int16_t discardBuffer[256];

static volatile size_t samplesRead = 0;
static volatile bool recording = false;
static volatile bool recordingReady = false;

static float helpScore = 0.0f;
static float noiseScore = 0.0f;
static float unknownScore = 0.0f;
static float bestOtherScore = 0.0f;

static bool helpDetected = false;
static uint8_t helpCandidateStreak = 0;

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

static bool recordAudioSample() {
    flushPdmBuffer();

    samplesRead = 0;
    recordingReady = false;
    recording = true;

    unsigned long startTime = millis();
    unsigned long expectedMs =
        (1000UL * EI_CLASSIFIER_RAW_SAMPLE_COUNT) / EI_CLASSIFIER_FREQUENCY;
    unsigned long timeoutMs = expectedMs + 1500;

    while (!recordingReady) {
        if (millis() - startTime > timeoutMs) {
            recording = false;
            return false;
        }
        delay(5);
    }

    return true;
}

static bool isHelpLabel(const char *label) {
    return strcmp(label, "AJUTOR") == 0 || strcmp(label, "ajutor") == 0;
}

static bool isNoiseLabel(const char *label) {
    return strcmp(label, "NOISE") == 0 || strcmp(label, "noise") == 0;
}

static bool isUnknownLabel(const char *label) {
    return strcmp(label, "UNKNOWN") == 0 || strcmp(label, "unknown") == 0;
}

static bool runAudioClassifier() {
    helpScore = 0.0f;
    noiseScore = 0.0f;
    unknownScore = 0.0f;
    bestOtherScore = 0.0f;
    helpDetected = false;

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &getSignalData;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    if (res != EI_IMPULSE_OK) {
        return false;
    }

    Serial.println("Classification scores:");

    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        const char *label = result.classification[i].label;
        float value = result.classification[i].value;

        Serial.print(label);
        Serial.print(": ");
        Serial.println(value, 4);

        if (isHelpLabel(label)) {
            helpScore = value;
        } else {
            if (value > bestOtherScore) {
                bestOtherScore = value;
            }
        }

        if (isNoiseLabel(label)) {
            noiseScore = value;
        }

        if (isUnknownLabel(label)) {
            unknownScore = value;
        }
    }

    float margin = helpScore - bestOtherScore;
    bool topIsHelp = helpScore > bestOtherScore;

    bool strongHelp =
        topIsHelp &&
        helpScore >= HELP_STRONG_THRESHOLD &&
        margin >= HELP_STRONG_MARGIN;

    bool candidateHelp =
        topIsHelp &&
        helpScore >= HELP_CANDIDATE_THRESHOLD &&
        margin >= HELP_MARGIN;

    if (candidateHelp) {
        if (helpCandidateStreak < 255) {
            helpCandidateStreak++;
        }
    } else {
        helpCandidateStreak = 0;
    }

    // helpDetected = strongHelp || (helpCandidateStreak >= HELP_CONFIRM_COUNT);

    helpDetected = strongHelp || candidateHelp;
    Serial.print("helpScore: ");
    Serial.println(helpScore, 4);

    Serial.print("bestOtherScore: ");
    Serial.println(bestOtherScore, 4);

    Serial.print("margin: ");
    Serial.println(margin, 4);

    Serial.print("candidateStreak: ");
    Serial.println(helpCandidateStreak);

    if (helpDetected) {
        helpCandidateStreak = 0;
    }

    return true;
}

bool audio_ml_init() {
    helpScore = 0.0f;
    helpDetected = false;
    PDM.onReceive(onPdmData);
    PDM.setGain(127);
    return PDM.begin(1, EI_CLASSIFIER_FREQUENCY);
}

bool audio_ml_update() {
    if (!recordAudioSample()) {
        helpScore = 0.0f;
        helpDetected = false;
        return false;
    }

    return runAudioClassifier();
}

float audio_ml_getHelpScore() {
    return helpScore;
}

bool audio_ml_isHelpDetected() {
    return helpDetected;
}
