#include <Arduino.h>

#include "audio/audio_ml.h"

static const unsigned long PREPARE_DELAY_MS = 1000;
static const unsigned long BETWEEN_TRIALS_MS = 1200;
static const int WINDOWS_PER_TRIAL = 3;

void printMenu() {
    Serial.println();
    Serial.println("=== Audio ML guided test ===");
    Serial.println("Scrie o comanda de forma: TEST NR_INCERCARI");
    Serial.println();
    Serial.println("Exemple:");
    Serial.println("LINISTE 20");
    Serial.println("ZGOMOT 20");
    Serial.println("VORBIRE_RANDOM 20");
    Serial.println("AJUTOR_APROAPE 20");
    Serial.println("AJUTOR_1M 20");
    Serial.println("AJUTOR_INCET 10");
    Serial.println("FRAZE_AJUTOR 10");
    Serial.println();
    Serial.println("La final copiaza linia SUMMARY in Excel.");
    Serial.println("========================================");
}

void runTest(String testName, int trialCount) {
    if (trialCount <= 0) {
        Serial.println("ERR: numar invalid de incercari");
        return;
    }

    int detectedCount = 0;

    Serial.println();
    Serial.print("TEST START: ");
    Serial.print(testName);
    Serial.print(", incercari = ");
    Serial.println(trialCount);

    Serial.println("Format rezultat:");
    Serial.println("TRIAL;test;trialIndex;helpScore;bestOther;margin;rms;mean;min;max;clipPct;detected");

    for (int i = 1; i <= trialCount; i++) {
        Serial.println();
        Serial.print("Pregateste incercarea ");
        Serial.print(i);
        Serial.print("/");
        Serial.println(trialCount);

        delay(PREPARE_DELAY_MS);

        Serial.println("START recording...");

        bool trialOk = false;
        bool detected = false;
        float helpScore = 0.0f;
        float bestOtherScore = 0.0f;
        float margin = 0.0f;
        float rms = 0.0f;
        float mean = 0.0f;
        int16_t minSample = 0;
        int16_t maxSample = 0;
        float clippingPercent = 0.0f;

        for (int windowIndex = 1; windowIndex <= WINDOWS_PER_TRIAL; windowIndex++) {
            Serial.print("Window ");
            Serial.print(windowIndex);
            Serial.print("/");
            Serial.println(WINDOWS_PER_TRIAL);

            if (!audio_ml_update()) {
                Serial.println("ERR: audio ML window failed");
                continue;
            }

            trialOk = true;

            float currentHelpScore = audio_ml_getHelpScore();

            if (currentHelpScore > helpScore) {
                helpScore = currentHelpScore;
                bestOtherScore = audio_ml_getBestOtherScore();
                margin = audio_ml_getMargin();
                rms = audio_ml_getRms();
                mean = audio_ml_getMean();
                minSample = audio_ml_getMinSample();
                maxSample = audio_ml_getMaxSample();
                clippingPercent = audio_ml_getClippingPercent();
            }

            if (audio_ml_isHelpDetected()) {
                detected = true;
            }
        }

        if (!trialOk) {
            Serial.print("TRIAL;");
            Serial.print(testName);
            Serial.print(";");
            Serial.print(i);
            Serial.println(";ERR;ERR;ERR;ERR;ERR;ERR;ERR;ERR;0");

            Serial.println("ERR: audio ML update failed");
            delay(BETWEEN_TRIALS_MS);
            continue;
        }

        if (detected) {
            detectedCount++;
        }

        Serial.print("TRIAL;");
        Serial.print(testName);
        Serial.print(";");
        Serial.print(i);
        Serial.print(";");
        Serial.print(helpScore, 4);
        Serial.print(";");
        Serial.print(bestOtherScore, 4);
        Serial.print(";");
        Serial.print(margin, 4);
        Serial.print(";");
        Serial.print(rms, 2);
        Serial.print(";");
        Serial.print(mean, 2);
        Serial.print(";");
        Serial.print(minSample);
        Serial.print(";");
        Serial.print(maxSample);
        Serial.print(";");
        Serial.print(clippingPercent, 2);
        Serial.print(";");
        Serial.println(detected ? 1 : 0);

        Serial.print("Rezultat incercare: ");
        if (detected) {
            Serial.println("AJUTOR DETECTAT");
        } else {
            Serial.println("none");
        }

        delay(BETWEEN_TRIALS_MS);
    }

    Serial.println();
    Serial.print("SUMMARY;");
    Serial.print(testName);
    Serial.print(";");
    Serial.print(trialCount);
    Serial.print(";");
    Serial.println(detectedCount);

    Serial.println();
    Serial.println("Test terminat. Scrie urmatoarea comanda.");
}

bool parseCommand(String command, String &testName, int &trialCount) {
    command.trim();

    if (command.length() == 0) {
        return false;
    }

    command.toUpperCase();

    if (command == "HELP" || command == "MENU") {
        printMenu();
        return false;
    }

    int spaceIndex = command.indexOf(' ');

    if (spaceIndex < 0) {
        Serial.println("ERR: comanda trebuie sa fie de forma TEST NR_INCERCARI");
        Serial.println("Exemplu: LINISTE 20");
        return false;
    }

    testName = command.substring(0, spaceIndex);

    String trialText = command.substring(spaceIndex + 1);
    trialText.trim();

    trialCount = trialText.toInt();

    if (trialCount <= 0) {
        Serial.println("ERR: numar de incercari invalid");
        return false;
    }

    return true;
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    delay(2000);

    while (!Serial && millis() < 5000) {
        delay(10);
    }

    Serial.println("Edge Impulse audio test");

    if (!audio_ml_init()) {
        Serial.println("ERR: audio ML init failed");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("Audio ML OK");
    printMenu();
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');

        String testName;
        int trialCount = 0;

        if (parseCommand(command, testName, trialCount)) {
            runTest(testName, trialCount);
        }
    }
}
