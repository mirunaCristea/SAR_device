#include <Arduino.h>
#include "audio/audio.h"

// ===================== CONFIGURARE TEST =====================

static const uint8_t NUM_PROBE = 20;

// ===================== DATE TEST =====================

struct AudioTestScenario {
    const char* name;
    bool expectedHelp;
};

static AudioTestScenario currentScenario = {"NESELECTAT", false};

static bool testActive = false;
static uint8_t probeCount = 0;
static uint8_t helpDetections = 0;
static uint8_t correctCount = 0;
static uint8_t falseAlarmCount = 0;
static uint8_t missedCount = 0;

static AudioData audioData;

// ===================== FUNCTII AUXILIARE =====================

void printMenu()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println(" TEST AUDIO TinyML - COMENZI DISPONIBILE");
    Serial.println("========================================");
    Serial.println("Trimite in Serial Monitor una dintre comenzile:");
    Serial.println("1 / AJUTOR     -> test apel vocal „ajutor”");
    Serial.println("2 / VORBIRE    -> test vorbire normala");
    Serial.println("3 / ZGOMOT     -> test zgomot ambiental");
    Serial.println("4 / AJUTATI    -> test expresie „ajutati-ma”");
    Serial.println();
    Serial.println("Setari Serial Monitor:");
    Serial.println("- Baud rate: 115200");
    Serial.println("- Line ending: Newline sau Both NL & CR");
    Serial.println("========================================");
    Serial.println();
}

void resetStats()
{
    probeCount = 0;
    helpDetections = 0;
    correctCount = 0;
    falseAlarmCount = 0;
    missedCount = 0;
}

void startScenario(const char* name, bool expectedHelp)
{
    currentScenario.name = name;
    currentScenario.expectedHelp = expectedHelp;

    resetStats();
    testActive = true;

    Serial.println();
    Serial.println("========================================");
    Serial.println(" PORNIRE TEST AUDIO TinyML");
    Serial.println("========================================");
    Serial.print("Scenariu: ");
    Serial.println(currentScenario.name);
    Serial.print("Nr. probe: ");
    Serial.println(NUM_PROBE);
    Serial.print("Se asteapta detectie HELP: ");
    Serial.println(currentScenario.expectedHelp ? "DA" : "NU");
    Serial.println("----------------------------------------");
    Serial.println("Vorbeste / genereaza sunetul pentru scenariul ales.");
    Serial.println("O proba este inregistrata la fiecare rezultat audio valid.");
    Serial.println("========================================");
    Serial.println();
}

void printResultForProbe(bool detectedHelp, uint8_t score)
{
    probeCount++;

    if (detectedHelp) {
        helpDetections++;
    }

    bool correct =
        (currentScenario.expectedHelp && detectedHelp) ||
        (!currentScenario.expectedHelp && !detectedHelp);

    if (correct) {
        correctCount++;
    }

    if (!currentScenario.expectedHelp && detectedHelp) {
        falseAlarmCount++;
    }

    if (currentScenario.expectedHelp && !detectedHelp) {
        missedCount++;
    }

    Serial.println("----------------------------------------");
    Serial.print("Scenariu: ");
    Serial.println(currentScenario.name);

    Serial.print("Proba: ");
    Serial.print(probeCount);
    Serial.print("/");
    Serial.println(NUM_PROBE);

    Serial.print("Rezultat model: ");
    Serial.println(detectedHelp ? "HELP" : "NORMAL");

    Serial.print("Scor HELP: ");
    Serial.print(score);
    Serial.println("%");

    Serial.print("Decizie proba: ");

    if (currentScenario.expectedHelp) {
        Serial.println(detectedHelp ? "DETECTIE CORECTA" : "RATARE");
    } else {
        Serial.println(detectedHelp ? "ALARMA FALSA" : "OK");
    }
}

void printSummary()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println(" REZUMAT TEST AUDIO TinyML");
    Serial.println("========================================");

    Serial.print("Scenariu: ");
    Serial.println(currentScenario.name);

    Serial.print("Probe totale: ");
    Serial.println(probeCount);

    Serial.print("Detectii HELP: ");
    Serial.print(helpDetections);
    Serial.print("/");
    Serial.println(probeCount);

    Serial.print("Clasificari corecte: ");
    Serial.print(correctCount);
    Serial.print("/");
    Serial.println(probeCount);

    float accuracy = 0.0f;
    if (probeCount > 0) {
        accuracy = (100.0f * correctCount) / probeCount;
    }

    Serial.print("Rata corectitudine: ");
    Serial.print(accuracy, 1);
    Serial.println("%");

    if (currentScenario.expectedHelp) {
        Serial.print("Ratări: ");
        Serial.println(missedCount);
    } else {
        Serial.print("Alarme false: ");
        Serial.println(falseAlarmCount);
    }

    Serial.println("========================================");
    Serial.println();

    testActive = false;
    printMenu();
}

void handleCommand(String cmd)
{
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "1" || cmd == "AJUTOR") {
        startScenario("AJUTOR", true);
    }
    else if (cmd == "2" || cmd == "VORBIRE") {
        startScenario("VORBIRE NORMALA", false);
    }
    else if (cmd == "3" || cmd == "ZGOMOT") {
        startScenario("ZGOMOT AMBIENTAL", false);
    }
    else if (cmd == "4" || cmd == "AJUTATI") {
        startScenario("AJUTATI-MA", true);
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

    if (audio_update(audioData)) {
        bool detectedHelp =
            audioData.valid &&
            audioData.state == AUDIO_HELP_DETECTED;

        printResultForProbe(detectedHelp, audioData.helpScore);

        if (probeCount >= NUM_PROBE) {
            printSummary();
        }
    }
}