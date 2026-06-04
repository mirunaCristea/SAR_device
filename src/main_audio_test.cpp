#include <Arduino.h>

#include "audio/audio_ml.h"

static const unsigned long PREPARE_DELAY_MS = 1000;
static const unsigned long BETWEEN_TRIALS_MS = 1200;

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
    Serial.println("TRIAL;test;trialIndex;helpScore;detected");

    for (int i = 1; i <= trialCount; i++) {
        Serial.println();
        Serial.print("Pregateste incercarea ");
        Serial.print(i);
        Serial.print("/");
        Serial.println(trialCount);

        delay(PREPARE_DELAY_MS);

        Serial.println("START recording...");

        if (!audio_ml_update()) {
            Serial.print("TRIAL;");
            Serial.print(testName);
            Serial.print(";");
            Serial.print(i);
            Serial.println(";ERR;0");

            Serial.println("ERR: audio ML update failed");
            delay(BETWEEN_TRIALS_MS);
            continue;
        }

        float helpScore = audio_ml_getHelpScore();
        bool detected = audio_ml_isHelpDetected();

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
    
    Serial.begin(115200);
    

    while (!Serial && millis() < 5000) {
        delay(10);
        Serial.println(".");
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