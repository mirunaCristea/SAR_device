#include <Arduino.h>
#include <string.h>
#include "audio/audio.h"

// ===================== CONFIGURARE TEST =====================

static const uint8_t NUM_EVENTS = 20;

static const unsigned long PRE_EVENT_PAUSE_MS = 2000;
static const unsigned long EVENT_DURATION_MS = 6000;
static const unsigned long POST_EVENT_OBSERVATION_MS = 3000;

static const unsigned long PASSIVE_TEST_DURATION_MS = 60000;

// Episod pozitiv / negativ detectat daca:
// 1) exista cel putin o fereastra strong
// SAU
// 2) minimum 25% din ferestre sunt candidate
static const float EVENT_HELP_RATIO_THRESHOLD = 0.25f;

static const bool PRINT_NORMAL_WINDOWS = false;

// ===================== DATE TEST =====================

struct AudioTestScenario {
    const char* name;
    bool expectedHelp;
};

enum TestMode {
    MODE_NONE,
    MODE_HELP_EVENTS,
    MODE_PASSIVE_NO_HELP
};

enum TestPhase {
    TEST_IDLE,
    PASSIVE_RUNNING,
    WAIT_BEFORE_EVENT,
    EVENT_ACTIVE,
    POST_EVENT_OBSERVATION
};

static AudioTestScenario currentScenario = {"NESELECTAT", false};

static TestMode testMode = MODE_NONE;
static TestPhase testPhase = TEST_IDLE;

static bool testActive = false;

static uint8_t eventCount = 0;
static uint8_t detectedEvents = 0;
static uint8_t missedEvents = 0;
static uint8_t falseAlarmCount = 0;

static uint16_t audioWindowCount = 0;

// Pentru episodul curent
static uint8_t eventWindowCount = 0;
static uint8_t eventCandidateWindowCount = 0;
static uint8_t eventStrongWindowCount = 0;

// Pentru test pasiv
static uint16_t passiveCandidateWindowCount = 0;
static uint16_t passiveStrongWindowCount = 0;

static bool firstHelpWindowSeen = false;

static unsigned long phaseStartTime = 0;
static unsigned long eventStartTime = 0;
static unsigned long testStartTime = 0;

static unsigned long firstDetectionDelayMs = 0;
static unsigned long sumDetectionDelayMs = 0;

static AudioData audioData;

// ===================== MENIU =====================

void printMenu()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println(" TEST AUDIO TinyML - COMENZI");
    Serial.println("========================================");
    Serial.println("1 / AJUTOR  -> test pe episoade: Ajutor");
    Serial.println("4 / AJUTATI -> test pe episoade: Ajutati-ma");
    Serial.println("2 / VORBIRE -> 60 s vorbire normala, fara ajutor");
    Serial.println("3 / ZGOMOT  -> 60 s zgomot ambiental, fara ajutor");
    Serial.println();
    Serial.println("Serial Monitor:");
    Serial.println("- Baud rate: 115200");
    Serial.println("- Line ending: Newline sau Both NL & CR");
    Serial.println("========================================");
    Serial.println();
}

// ===================== RESET =====================

void resetStats()
{
    eventCount = 0;
    detectedEvents = 0;
    missedEvents = 0;
    falseAlarmCount = 0;

    audioWindowCount = 0;

    eventWindowCount = 0;
    eventCandidateWindowCount = 0;
    eventStrongWindowCount = 0;

    passiveCandidateWindowCount = 0;
    passiveStrongWindowCount = 0;

    firstHelpWindowSeen = false;

    phaseStartTime = 0;
    eventStartTime = 0;
    testStartTime = 0;

    firstDetectionDelayMs = 0;
    sumDetectionDelayMs = 0;
}

// ===================== START TESTE =====================

void startHelpScenario(const char* name)
{
    currentScenario.name = name;
    currentScenario.expectedHelp = true;

    resetStats();

    testMode = MODE_HELP_EVENTS;
    testPhase = WAIT_BEFORE_EVENT;
    testActive = true;

    phaseStartTime = millis();

    Serial.println();
    Serial.println("========================================");
    Serial.println(" TEST REALIST APEL DE AJUTOR");
    Serial.println("========================================");
    Serial.print("Scenariu: ");
    Serial.println(currentScenario.name);
    Serial.print("Numar episoade: ");
    Serial.println(NUM_EVENTS);

    Serial.println();
    Serial.println("Fiecare episod are:");
    Serial.print("- ");
    Serial.print(PRE_EVENT_PAUSE_MS / 1000);
    Serial.println(" s pauza");
    Serial.print("- ");
    Serial.print(EVENT_DURATION_MS / 1000);
    Serial.println(" s apel vocal");
    Serial.print("- ");
    Serial.print(POST_EVENT_OBSERVATION_MS / 1000);
    Serial.println(" s observare");

    Serial.println();
    Serial.println("Episod detectat daca:");
    Serial.println("- exista minimum o fereastra STRONG");
    Serial.print("- SAU minimum ");
    Serial.print(EVENT_HELP_RATIO_THRESHOLD * 100.0f, 1);
    Serial.println("% din ferestre sunt CANDIDATE");

    Serial.println("========================================");
}

void startPassiveScenario(const char* name)
{
    currentScenario.name = name;
    currentScenario.expectedHelp = false;

    resetStats();

    testMode = MODE_PASSIVE_NO_HELP;
    testPhase = PASSIVE_RUNNING;
    testActive = true;

    testStartTime = millis();
    phaseStartTime = millis();

    Serial.println();
    Serial.println("========================================");
    Serial.println(" TEST PASIV - FARA APEL DE AJUTOR");
    Serial.println("========================================");
    Serial.print("Scenariu: ");
    Serial.println(currentScenario.name);
    Serial.print("Durata: ");
    Serial.print(PASSIVE_TEST_DURATION_MS / 1000);
    Serial.println(" s");

    Serial.println();
    Serial.println("Testul pasiv este tratat ca un episod negativ.");
    Serial.println("Alarma falsa finala apare daca:");
    Serial.println("- exista minimum o fereastra STRONG");
    Serial.print("- SAU minimum ");
    Serial.print(EVENT_HELP_RATIO_THRESHOLD * 100.0f, 1);
    Serial.println("% din ferestre sunt CANDIDATE");

    Serial.println("========================================");
}

// ===================== PROCESARE FEREASTRA AUDIO =====================

void processAudioWindow(bool candidateHelp, bool strongHelp, uint8_t score)
{
    audioWindowCount++;

    bool insideHelpEvent =
        testMode == MODE_HELP_EVENTS &&
        (testPhase == EVENT_ACTIVE || testPhase == POST_EVENT_OBSERVATION);

    if (insideHelpEvent) {
        eventWindowCount++;

        if (candidateHelp) {
            eventCandidateWindowCount++;
        }

        if (strongHelp) {
            eventStrongWindowCount++;
        }

        if ((candidateHelp || strongHelp) && !firstHelpWindowSeen) {
            firstHelpWindowSeen = true;
            firstDetectionDelayMs = millis() - eventStartTime;

            Serial.println();
            Serial.println(">>> PRIMA FEREASTRA HELP IN EPISOD");
            Serial.print("Eveniment: ");
            Serial.print(eventCount);
            Serial.print("/");
            Serial.println(NUM_EVENTS);

            Serial.print("Scor HELP: ");
            Serial.print(score);
            Serial.println("%");

            Serial.print("Tip fereastra: ");
            Serial.println(strongHelp ? "STRONG" : "CANDIDATE");

            Serial.print("Timp pana la prima fereastra HELP: ");
            Serial.print(firstDetectionDelayMs);
            Serial.println(" ms");
        }

        return;
    }

    if (testMode == MODE_PASSIVE_NO_HELP) {
        if (candidateHelp) {
            passiveCandidateWindowCount++;
        }

        if (strongHelp) {
            passiveStrongWindowCount++;
        }

        if (PRINT_NORMAL_WINDOWS && !candidateHelp && !strongHelp) {
            Serial.print(".");
        }

        return;
    }

    // Pauza dintre evenimentele de ajutor
    if (strongHelp) {
        Serial.println();
        Serial.println("!!! FEREASTRA STRONG IN PAUZA");
        Serial.print("Scor HELP: ");
        Serial.print(score);
        Serial.println("%");
    } else if (PRINT_NORMAL_WINDOWS) {
        Serial.print(".");
    }
}

// ===================== DECIZIE EPISOD =====================

bool decideEpisode(
    uint16_t totalWindows,
    uint16_t candidateWindows,
    uint16_t strongWindows,
    float &candidateRatio
)
{
    candidateRatio = 0.0f;

    if (totalWindows > 0) {
        candidateRatio = candidateWindows / (float)totalWindows;
    }

    bool detectedByStrong =
        strongWindows >= 1;

    bool detectedByRatio =
        totalWindows > 0 &&
        candidateRatio >= EVENT_HELP_RATIO_THRESHOLD;

    return detectedByStrong || detectedByRatio;
}

// ===================== REZUMAT =====================

void printSummary()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println(" REZUMAT TEST AUDIO TinyML");
    Serial.println("========================================");

    Serial.print("Scenariu: ");
    Serial.println(currentScenario.name);

    Serial.print("Ferestre audio analizate: ");
    Serial.println(audioWindowCount);

    if (testMode == MODE_HELP_EVENTS) {
        Serial.print("Evenimente reale: ");
        Serial.println(eventCount);

        Serial.print("Evenimente detectate: ");
        Serial.print(detectedEvents);
        Serial.print("/");
        Serial.println(eventCount);

        Serial.print("Evenimente ratate: ");
        Serial.println(missedEvents);

        float detectionRate = 0.0f;

        if (eventCount > 0) {
            detectionRate =
                100.0f * detectedEvents / (float)eventCount;
        }

        Serial.print("Rata detectie pe eveniment: ");
        Serial.print(detectionRate, 1);
        Serial.println("%");

        if (detectedEvents > 0) {
            float avgDelay =
                sumDetectionDelayMs / (float)detectedEvents;

            Serial.print("Timp mediu pana la prima fereastra HELP: ");
            Serial.print(avgDelay, 1);
            Serial.println(" ms");
        }

        Serial.print("Alarme false in pauze: ");
        Serial.println(falseAlarmCount);
    }

    if (testMode == MODE_PASSIVE_NO_HELP) {
        float ratio = 0.0f;

        bool passiveFalseAlarm =
            decideEpisode(
                audioWindowCount,
                passiveCandidateWindowCount,
                passiveStrongWindowCount,
                ratio
            );

        falseAlarmCount = passiveFalseAlarm ? 1 : 0;

        Serial.print("Ferestre CANDIDATE false: ");
        Serial.println(passiveCandidateWindowCount);

        Serial.print("Ferestre STRONG false: ");
        Serial.println(passiveStrongWindowCount);

        Serial.print("Procent ferestre CANDIDATE false: ");
        Serial.print(ratio * 100.0f, 1);
        Serial.println("%");

        Serial.print("Alarma falsa finala: ");
        Serial.println(passiveFalseAlarm ? "DA" : "NU");
    }

    Serial.println("========================================");
    Serial.println();

    testActive = false;
    testMode = MODE_NONE;
    testPhase = TEST_IDLE;

    printMenu();
}

// ===================== TIMING TEST PE EPISOADE =====================

void updateHelpScenarioTiming()
{
    unsigned long now = millis();

    if (testPhase == WAIT_BEFORE_EVENT) {
        if (now - phaseStartTime >= PRE_EVENT_PAUSE_MS) {
            eventCount++;

            eventWindowCount = 0;
            eventCandidateWindowCount = 0;
            eventStrongWindowCount = 0;

            firstHelpWindowSeen = false;
            firstDetectionDelayMs = 0;

            eventStartTime = now;
            phaseStartTime = now;
            testPhase = EVENT_ACTIVE;

            Serial.println();
            Serial.println("========================================");
            Serial.print(" EVENIMENT ");
            Serial.print(eventCount);
            Serial.print("/");
            Serial.println(NUM_EVENTS);

            if (strcmp(currentScenario.name, "AJUTOR") == 0) {
                Serial.println(" STRIGA ACUM: Ajutor! Ajutor! Ajutor!");
            } else {
                Serial.println(" STRIGA ACUM: Ajutati-ma! Ajutati-ma!");
            }

            Serial.println("========================================");
        }
    }

    else if (testPhase == EVENT_ACTIVE) {
        if (now - phaseStartTime >= EVENT_DURATION_MS) {
            phaseStartTime = now;
            testPhase = POST_EVENT_OBSERVATION;

            Serial.println("Gata apelul. Asteapta observarea...");
        }
    }

    else if (testPhase == POST_EVENT_OBSERVATION) {
        if (now - phaseStartTime >= POST_EVENT_OBSERVATION_MS) {
            float ratio = 0.0f;

            bool eventDetected =
                decideEpisode(
                    eventWindowCount,
                    eventCandidateWindowCount,
                    eventStrongWindowCount,
                    ratio
                );

            Serial.print("Ferestre in episod: ");
            Serial.println(eventWindowCount);

            Serial.print("Ferestre CANDIDATE: ");
            Serial.println(eventCandidateWindowCount);

            Serial.print("Ferestre STRONG: ");
            Serial.println(eventStrongWindowCount);

            Serial.print("Procent CANDIDATE: ");
            Serial.print(ratio * 100.0f, 1);
            Serial.println("%");

            if (eventDetected) {
                detectedEvents++;

                if (firstHelpWindowSeen) {
                    sumDetectionDelayMs += firstDetectionDelayMs;
                }

                Serial.println("Rezultat eveniment: DETECTAT");
            } else {
                missedEvents++;
                Serial.println("Rezultat eveniment: RATAT");
            }

            if (eventCount >= NUM_EVENTS) {
                printSummary();
            } else {
                phaseStartTime = now;
                testPhase = WAIT_BEFORE_EVENT;

                Serial.println();
                Serial.println("----------------------------------------");
                Serial.println("Pauza scurta. Nu spune ajutor.");
            }
        }
    }
}

// ===================== TIMING TEST PASIV =====================

void updatePassiveScenarioTiming()
{
    unsigned long now = millis();

    if (now - testStartTime >= PASSIVE_TEST_DURATION_MS) {
        printSummary();
    }
}

void updateTestTiming()
{
    if (!testActive) {
        return;
    }

    if (testMode == MODE_HELP_EVENTS) {
        updateHelpScenarioTiming();
    }

    else if (testMode == MODE_PASSIVE_NO_HELP) {
        updatePassiveScenarioTiming();
    }
}

// ===================== COMENZI =====================

void handleCommand(String cmd)
{
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "1" || cmd == "AJUTOR") {
        startHelpScenario("AJUTOR");
    }
    else if (cmd == "4" || cmd == "AJUTATI") {
        startHelpScenario("AJUTATI-MA");
    }
    else if (cmd == "2" || cmd == "VORBIRE") {
        startPassiveScenario("VORBIRE NORMALA");
    }
    else if (cmd == "3" || cmd == "ZGOMOT") {
        startPassiveScenario("ZGOMOT AMBIENTAL");
    }
    else {
        Serial.println("Comanda necunoscuta.");
        printMenu();
    }
}

// ===================== SETUP =====================

void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("Initializare test audio TinyML...");

    audio_init();

    Serial.println("Initializare finalizata.");
    printMenu();
}

// ===================== LOOP =====================

void loop()
{
    if (Serial.available() > 0) {
        String cmd = Serial.readStringUntil('\n');
        handleCommand(cmd);
    }

    if (!testActive) {
        return;
    }

    updateTestTiming();

    if (testActive && audio_update(audioData)) {
        bool candidateHelp =
            audioData.valid &&
            audioData.helpCandidate;

        bool strongHelp =
            audioData.valid &&
            audioData.helpStrong;

        processAudioWindow(
            candidateHelp,
            strongHelp,
            audioData.helpScore
        );
    }

    updateTestTiming();
}