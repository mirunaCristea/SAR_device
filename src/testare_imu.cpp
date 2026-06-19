#include <Arduino.h>
#include <string.h>
#include "imu/imu.h"

// ===================== CONFIGURARE TEST =====================
static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long LIVE_PRINT_INTERVAL_MS = 250;
static const int MAX_TRIALS = 30;

// ===================== DATE IMU =====================
static IMUdata imuData;

static unsigned long lastImuTime = 0;
static unsigned long lastLivePrintTime = 0;

static const char* currentTest = "TEST_NESELECTAT";
static bool currentExpectedFall = false;

static bool trialActive = false;
static int nextTrialId = 1;

// ===================== STRUCTURI TEST =====================
struct TrialStats {
    unsigned long startMs = 0;
    unsigned long endMs = 0;

    uint32_t sampleCount = 0;
    uint32_t stationaryCount = 0;
    uint32_t movingCount = 0;
    uint32_t possibleFallCount = 0;
    uint32_t impactCount = 0;
    uint32_t confirmedFallCount = 0;

    bool fallDetected = false;

    float minAsvm = 999.0f;
    float maxAsvm = 0.0f;
    float maxGsvm = 0.0f;
};

struct TrialResult {
    int id = 0;
    char testName[45];

    bool expectedFall = false;
    bool fallDetected = false;

    float durationS = 0.0f;

    uint32_t sampleCount = 0;
    uint32_t stationaryCount = 0;
    uint32_t movingCount = 0;
    uint32_t possibleFallCount = 0;
    uint32_t impactCount = 0;
    uint32_t confirmedFallCount = 0;

    float minAsvm = 0.0f;
    float maxAsvm = 0.0f;
    float maxGsvm = 0.0f;
};

static TrialStats currentStats;
static TrialResult results[MAX_TRIALS];
static int resultCount = 0;

static MotionState lastMotionState = MOTION_UNKNOWN;
static ImuState lastImuState = IMU_NORMAL;

// ===================== CONVERSII TEXT =====================
const char* motionToText(MotionState state)
{
    switch (state) {
        case STATIONARY: return "STATIONAR";
        case MOVING:     return "IN_MISCARE";
        case IMMOBILE:   return "IMOBIL";
        default:         return "MISCARE_NECUNOSCUTA";
    }
}

const char* imuStateToText(ImuState state)
{
    switch (state) {
        case IMU_NORMAL:         return "NORMAL";
        case IMU_POSSIBLE_FALL:  return "ANALIZA_CADERE";
        case IMU_IMPACT:         return "IMPACT";
        case IMU_CONFIRMED_FALL: return "CADERE_CONFIRMATA";
        default:                 return "STARE_IMU_NECUNOSCUTA";
    }
}

const char* daNu(bool value)
{
    return value ? "DA" : "NU";
}

const char* expectedToText(bool expectedFall)
{
    return expectedFall ? "CADERE" : "NU_CADERE";
}

const char* verdictToText(const TrialResult& r)
{
    if (r.expectedFall && r.fallDetected) {
        return "CORECT_DETECTATA";
    }

    if (r.expectedFall && !r.fallDetected) {
        return "CADERE_RATATA";
    }

    if (!r.expectedFall && r.fallDetected) {
        return "ALARMA_FALSA";
    }

    return "CORECT_FARA_ALARMA";
}

// ===================== RESET STATISTICI =====================
void resetTrialStats()
{
    currentStats = TrialStats();

    lastMotionState = MOTION_UNKNOWN;
    lastImuState = IMU_NORMAL;
}

// ===================== MENIU =====================
void printMenu()
{
    Serial.println();
    Serial.println("========== TEST IMU PE INCERCARI ==========");
    Serial.println("Selectare scenariu:");
    Serial.println("1 = Dispozitiv nemiscat pe masa");
    Serial.println("2 = Mers / miscare normala");
    Serial.println("3 = Miscare brusca fara cadere");
    Serial.println("4 = Cadere simulata pe material moale");
    Serial.println();
    Serial.println("Control incercare:");
    Serial.println("s = Start incercare");
    Serial.println("e = End incercare + salvare in tabel");
    Serial.println("r = Reset incercare curenta, fara salvare");
    Serial.println();
    Serial.println("Tabel:");
    Serial.println("t = Afiseaza tabel rezultate");
    Serial.println("a = Afiseaza statistici agregate");
    Serial.println("R = Reset tabel complet");
    Serial.println("h = Afiseaza meniul");
    Serial.println("===========================================");
    Serial.println();
}

// ===================== SELECTARE TEST =====================
void selectTest(const char* testName, bool expectedFall)
{
    if (trialActive) {
        Serial.println("ATENTIE: O incercare este activa. Apasa e sau r inainte de a schimba testul.");
        return;
    }

    currentTest = testName;
    currentExpectedFall = expectedFall;

    Serial.println();
    Serial.println("======================================");
    Serial.print("TEST SELECTAT: ");
    Serial.println(currentTest);

    Serial.print("Rezultat asteptat: ");
    Serial.println(expectedToText(currentExpectedFall));

    Serial.println("Apasa s pentru a porni incercarea.");
    Serial.println("======================================");
    Serial.println();
}

// ===================== START / END INCERCARE =====================
void startTrial()
{
    if (strcmp(currentTest, "TEST_NESELECTAT") == 0) {
        Serial.println("Selecteaza mai intai un test: 1, 2, 3 sau 4.");
        return;
    }

    if (trialActive) {
        Serial.println("Exista deja o incercare activa. Apasa e pentru salvare sau r pentru reset.");
        return;
    }

    resetTrialStats();

    currentStats.startMs = millis();
    trialActive = true;

    Serial.println();
    Serial.println("---------- START INCERCARE ----------");
    Serial.print("ID incercare: ");
    Serial.println(nextTrialId);

    Serial.print("Test: ");
    Serial.println(currentTest);

    Serial.print("Rezultat asteptat: ");
    Serial.println(expectedToText(currentExpectedFall));

    Serial.println("Executa miscarea/caderea acum.");
    Serial.println("Dupa eveniment, asteapta 2-3 secunde si apoi apasa e.");
    Serial.println("-------------------------------------");
    Serial.println();
}

void saveTrialResult()
{
    if (resultCount >= MAX_TRIALS) {
        Serial.println("Tabelul este plin. Apasa R pentru resetarea tabelului.");
        return;
    }

    TrialResult& r = results[resultCount];

    r.id = nextTrialId++;

    strncpy(r.testName, currentTest, sizeof(r.testName) - 1);
    r.testName[sizeof(r.testName) - 1] = '\0';

    r.expectedFall = currentExpectedFall;
    r.fallDetected = currentStats.fallDetected;

    r.durationS = (currentStats.endMs - currentStats.startMs) / 1000.0f;

    r.sampleCount = currentStats.sampleCount;
    r.stationaryCount = currentStats.stationaryCount;
    r.movingCount = currentStats.movingCount;
    r.possibleFallCount = currentStats.possibleFallCount;
    r.impactCount = currentStats.impactCount;
    r.confirmedFallCount = currentStats.confirmedFallCount;

    r.minAsvm = currentStats.minAsvm;
    r.maxAsvm = currentStats.maxAsvm;
    r.maxGsvm = currentStats.maxGsvm;

    resultCount++;
}

void endTrial()
{
    if (!trialActive) {
        Serial.println("Nu exista o incercare activa. Apasa s pentru start.");
        return;
    }

    currentStats.endMs = millis();
    trialActive = false;

    saveTrialResult();

    const TrialResult& r = results[resultCount - 1];

    Serial.println();
    Serial.println("========== REZULTAT INCERCARE ==========");
    Serial.print("ID: ");
    Serial.println(r.id);

    Serial.print("Test: ");
    Serial.println(r.testName);

    Serial.print("Durata [s]: ");
    Serial.println(r.durationS, 1);

    Serial.print("Rezultat asteptat: ");
    Serial.println(expectedToText(r.expectedFall));

    Serial.print("Cadere detectata: ");
    Serial.println(daNu(r.fallDetected));

    Serial.print("Impact count: ");
    Serial.println(r.impactCount);

    Serial.print("Cadere confirmata count: ");
    Serial.println(r.confirmedFallCount);

    Serial.print("asvm minim: ");
    Serial.println(r.minAsvm, 3);

    Serial.print("asvm maxim: ");
    Serial.println(r.maxAsvm, 3);

    Serial.print("gsvm maxim: ");
    Serial.println(r.maxGsvm, 2);

    Serial.print("Verdict: ");
    Serial.println(verdictToText(r));

    Serial.println("========================================");
    Serial.println();
}

// ===================== ACTUALIZARE STATISTICI =====================
void updateStats(const IMUdata& data)
{
    currentStats.sampleCount++;

    if (data.asvm < currentStats.minAsvm) {
        currentStats.minAsvm = data.asvm;
    }

    if (data.asvm > currentStats.maxAsvm) {
        currentStats.maxAsvm = data.asvm;
    }

    if (data.gsvm > currentStats.maxGsvm) {
        currentStats.maxGsvm = data.gsvm;
    }

    if (data.motionState == STATIONARY) {
        currentStats.stationaryCount++;
    }
    else if (data.motionState == MOVING) {
        currentStats.movingCount++;
    }

    if (data.imuState == IMU_POSSIBLE_FALL) {
        currentStats.possibleFallCount++;
    }
    else if (data.imuState == IMU_IMPACT) {
        currentStats.impactCount++;
    }
    else if (data.imuState == IMU_CONFIRMED_FALL) {
        currentStats.confirmedFallCount++;
    }

    if (data.fallFlag) {
        currentStats.fallDetected = true;
    }
}

// ===================== EVENIMENTE IMPORTANTE =====================
void printEventIfChanged(const IMUdata& data)
{
    bool motionChanged = data.motionState != lastMotionState;
    bool imuChanged = data.imuState != lastImuState;

    if (motionChanged || imuChanged || data.fallFlag) {
        Serial.print("EVENIMENT;test=");
        Serial.print(currentTest);
        Serial.print(";stare_miscare=");
        Serial.print(motionToText(data.motionState));
        Serial.print(";stare_imu=");
        Serial.print(imuStateToText(data.imuState));
        Serial.print(";cadere_detectata=");
        Serial.print(daNu(data.fallFlag));
        Serial.print(";asvm=");
        Serial.print(data.asvm, 3);
        Serial.print(";gsvm=");
        Serial.println(data.gsvm, 2);
    }

    lastMotionState = data.motionState;
    lastImuState = data.imuState;
}

// ===================== LIVE PRINT =====================
void printLiveLine(const IMUdata& data)
{
    Serial.print("LIVE;");
    Serial.print(currentTest);
    Serial.print(";t_ms=");
    Serial.print(millis());
    Serial.print(";asvm=");
    Serial.print(data.asvm, 3);
    Serial.print(";gsvm=");
    Serial.print(data.gsvm, 2);
    Serial.print(";miscare=");
    Serial.print(motionToText(data.motionState));
    Serial.print(";imu=");
    Serial.print(imuStateToText(data.imuState));
    Serial.print(";cadere=");
    Serial.println(daNu(data.fallFlag));
}

// ===================== TABEL REZULTATE =====================
void printResultsTable()
{
    Serial.println();
    Serial.println("========== TABEL REZULTATE ==========");
    Serial.println("id;test;asteptat;detectat;durata_s;esantioane;stationar;in_miscare;analiza_cadere;impact;cadere_confirmata;asvm_min;asvm_max;gsvm_max;verdict");

    for (int i = 0; i < resultCount; i++) {
        const TrialResult& r = results[i];

        Serial.print(r.id);
        Serial.print(";");

        Serial.print(r.testName);
        Serial.print(";");

        Serial.print(expectedToText(r.expectedFall));
        Serial.print(";");

        Serial.print(daNu(r.fallDetected));
        Serial.print(";");

        Serial.print(r.durationS, 1);
        Serial.print(";");

        Serial.print(r.sampleCount);
        Serial.print(";");

        Serial.print(r.stationaryCount);
        Serial.print(";");

        Serial.print(r.movingCount);
        Serial.print(";");

        Serial.print(r.possibleFallCount);
        Serial.print(";");

        Serial.print(r.impactCount);
        Serial.print(";");

        Serial.print(r.confirmedFallCount);
        Serial.print(";");

        Serial.print(r.minAsvm, 3);
        Serial.print(";");

        Serial.print(r.maxAsvm, 3);
        Serial.print(";");

        Serial.print(r.maxGsvm, 2);
        Serial.print(";");

        Serial.println(verdictToText(r));
    }

    Serial.println("=====================================");
    Serial.println();
}

// ===================== STATISTICI AGREGATE =====================
void printAggregateStats()
{
    int fallTests = 0;
    int detectedFalls = 0;

    int noFallTests = 0;
    int falseAlarms = 0;

    for (int i = 0; i < resultCount; i++) {
        const TrialResult& r = results[i];

        if (r.expectedFall) {
            fallTests++;
            if (r.fallDetected) {
                detectedFalls++;
            }
        }
        else {
            noFallTests++;
            if (r.fallDetected) {
                falseAlarms++;
            }
        }
    }

    Serial.println();
    Serial.println("========== STATISTICI AGREGATE ==========");

    Serial.print("Numar total incercari: ");
    Serial.println(resultCount);

    Serial.print("Caderi simulate: ");
    Serial.println(fallTests);

    Serial.print("Caderi detectate corect: ");
    Serial.println(detectedFalls);

    if (fallTests > 0) {
        float detectionRate = 100.0f * detectedFalls / fallTests;
        Serial.print("Rata detectie caderi [%]: ");
        Serial.println(detectionRate, 1);
    }

    Serial.print("Incercari fara cadere: ");
    Serial.println(noFallTests);

    Serial.print("Alarme false: ");
    Serial.println(falseAlarms);

    if (noFallTests > 0) {
        float falseAlarmRate = 100.0f * falseAlarms / noFallTests;
        Serial.print("Rata alarme false [%]: ");
        Serial.println(falseAlarmRate, 1);
    }

    Serial.println("=========================================");
    Serial.println();
}

// ===================== SERIAL =====================
void handleSerial()
{
    while (Serial.available() > 0) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            continue;
        }

        if (c == '1') {
            selectTest("DISPOZITIV_NEMISCAT_PE_MASA", false);
        }
        else if (c == '2') {
            selectTest("MERS_MISC_NORMALA", false);
        }
        else if (c == '3') {
            selectTest("MISCARE_BRUSCA_FARA_CADERE", false);
        }
        else if (c == '4') {
            selectTest("CADERE_SIMULATA_MATERIAL_MOALE", true);
        }
        else if (c == 's' || c == 'S') {
            startTrial();
        }
        else if (c == 'e' || c == 'E') {
            endTrial();
        }
        else if (c == 'r') {
            if (trialActive) {
                resetTrialStats();
                currentStats.startMs = millis();
                Serial.println("Incercarea curenta a fost resetata, fara salvare in tabel.");
            }
            else {
                Serial.println("Nu exista incercare activa de resetat.");
            }
        }
        else if (c == 't' || c == 'T') {
            printResultsTable();
        }
        else if (c == 'a' || c == 'A') {
            printAggregateStats();
        }
        else if (c == 'R') {
            resultCount = 0;
            nextTrialId = 1;
            trialActive = false;
            resetTrialStats();
            Serial.println("Tabelul complet a fost resetat.");
        }
        else if (c == 'h' || c == 'H') {
            printMenu();
        }
    }
}

// ===================== SETUP =====================
void setup()
{
    Serial.begin(115200);

    while (!Serial && millis() < 8000) {
        delay(10);
    }

    delay(1000);

    Serial.println("Pornire test IMU pe incercari controlate...");

    if (!IMU_init()) {
        Serial.println("EROARE: IMU nu a putut fi initializat.");
        while (true) {
            delay(1000);
        }
    }

    resetTrialStats();
    printMenu();
}

// ===================== LOOP =====================
void loop()
{
    handleSerial();

    if (!trialActive) {
        return;
    }

    unsigned long now = millis();

    if (now - lastImuTime >= IMU_INTERVAL_MS) {
        lastImuTime = now;

        if (IMU_read(imuData)) {
            imuData = IMU_interpret(imuData);
            updateStats(imuData);
            printEventIfChanged(imuData);
        }
    }

    if (now - lastLivePrintTime >= LIVE_PRINT_INTERVAL_MS) {
        lastLivePrintTime = now;
        printLiveLine(imuData);
    }
}