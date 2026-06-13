#include <Arduino.h>
#include "imu/imu.h"

// ===================== CONFIGURARE TEST =====================
static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long PRINT_INTERVAL_MS = 100;
static const unsigned long SUMMARY_INTERVAL_MS = 5000;

// ===================== DATE TEST =====================
static IMUdata imuData;

static unsigned long lastImuTime = 0;
static unsigned long lastPrintTime = 0;
static unsigned long lastSummaryTime = 0;
static unsigned long testStartTime = 0;

static const char* currentTest = "TEST_NESELECTAT";

// ===================== STATISTICI =====================
static uint32_t sampleCount = 0;
static uint32_t stationaryCount = 0;
static uint32_t movingCount = 0;
static uint32_t possibleFallCount = 0;
static uint32_t impactCount = 0;
static uint32_t confirmedFallCount = 0;

static bool fallDetectedInTest = false;

static float minAsvm = 999.0f;
static float maxAsvm = 0.0f;
static float maxGsvm = 0.0f;

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
        case IMU_POSSIBLE_FALL:  return "POSIBILA_CADERE";
        case IMU_IMPACT:         return "IMPACT";
        case IMU_CONFIRMED_FALL: return "CADERE_CONFIRMATA";
        default:                 return "STARE_IMU_NECUNOSCUTA";
    }
}

// ===================== RESET TEST =====================
void resetTestStats()
{
    sampleCount = 0;
    stationaryCount = 0;
    movingCount = 0;
    possibleFallCount = 0;
    impactCount = 0;
    confirmedFallCount = 0;

    fallDetectedInTest = false;

    minAsvm = 999.0f;
    maxAsvm = 0.0f;
    maxGsvm = 0.0f;

    lastMotionState = MOTION_UNKNOWN;
    lastImuState = IMU_NORMAL;

    testStartTime = millis();
}

void selectTest(const char* testName)
{
    currentTest = testName;
    resetTestStats();

    Serial.println();
    Serial.println("======================================");
    Serial.print("TEST SELECTAT: ");
    Serial.println(currentTest);
    Serial.println("Porneste incercarea si salveaza logul.");
    Serial.println("======================================");
    Serial.println();
}

// ===================== MENIU SERIAL =====================
void printMenu()
{
    Serial.println();
    Serial.println("========== TEST IMU: NORMAL VS CADERE ==========");
    Serial.println("Trimite in Serial Monitor:");
    Serial.println("1 = Dispozitiv nemiscat pe masa");
    Serial.println("2 = Mers / miscare normala");
    Serial.println("3 = Miscare brusca fara cadere");
    Serial.println("4 = Cadere simulata pe material moale");
    Serial.println("r = Reset incercare curenta");
    Serial.println("h = Afiseaza meniul");
    Serial.println();
    Serial.println("Format CSV:");
    Serial.println("test;timp_ms;asvm;gsvm;stare_miscare;stare_imu;cadere_detectata");
    Serial.println("================================================");
    Serial.println();
}

void handleSerial()
{
    while (Serial.available() > 0) {
        char c = Serial.read();

        if (c == '1') {
            selectTest("DISPOZITIV_NEMISCAT_PE_MASA");
        } 
        else if (c == '2') {
            selectTest("MERS_MISC_NORMALA");
        } 
        else if (c == '3') {
            selectTest("MISCARE_BRUSCA_FARA_CADERE");
        } 
        else if (c == '4') {
            selectTest("CADERE_SIMULATA_MATERIAL_MOALE");
        } 
        else if (c == 'r' || c == 'R') {
            resetTestStats();
        } 
        else if (c == 'h' || c == 'H') {
            printMenu();
        }
    }
}

// ===================== ACTUALIZARE STATISTICI =====================
void updateStats(const IMUdata& data)
{
    sampleCount++;

    if (data.asvm < minAsvm) {
        minAsvm = data.asvm;
    }

    if (data.asvm > maxAsvm) {
        maxAsvm = data.asvm;
    }

    if (data.gsvm > maxGsvm) {
        maxGsvm = data.gsvm;
    }

    if (data.motionState == STATIONARY) {
        stationaryCount++;
    } 
    else if (data.motionState == MOVING) {
        movingCount++;
    }

    if (data.imuState == IMU_POSSIBLE_FALL) {
        possibleFallCount++;
    } 
    else if (data.imuState == IMU_IMPACT) {
        impactCount++;
    } 
    else if (data.imuState == IMU_CONFIRMED_FALL) {
        confirmedFallCount++;
    }

    if (data.fallFlag) {
        fallDetectedInTest = true;
    }
}

// ===================== PRINT EVENIMENTE IMPORTANTE =====================
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
        Serial.print(data.fallFlag ? "DA" : "NU");
        Serial.print(";asvm=");
        Serial.print(data.asvm, 3);
        Serial.print(";gsvm=");
        Serial.println(data.gsvm, 2);
    }

    lastMotionState = data.motionState;
    lastImuState = data.imuState;
}

// ===================== PRINT CSV =====================
void printCsvLine(const IMUdata& data)
{
    Serial.print(currentTest);
    Serial.print(";");

    Serial.print(millis());
    Serial.print(";");

    Serial.print(data.asvm, 3);
    Serial.print(";");

    Serial.print(data.gsvm, 2);
    Serial.print(";");

    Serial.print(motionToText(data.motionState));
    Serial.print(";");

    Serial.print(imuStateToText(data.imuState));
    Serial.print(";");

    Serial.println(data.fallFlag ? "DA" : "NU");
}

// ===================== PRINT SUMAR =====================
void printSummary()
{
    float durationS = (millis() - testStartTime) / 1000.0f;

    Serial.println();
    Serial.println("---------- SUMAR INCERCARE ----------");

    Serial.print("Test: ");
    Serial.println(currentTest);

    Serial.print("Durata [s]: ");
    Serial.println(durationS, 1);

    Serial.print("Numar esantioane: ");
    Serial.println(sampleCount);

    Serial.print("Numar stari STATIONAR: ");
    Serial.println(stationaryCount);

    Serial.print("Numar stari IN_MISCARE: ");
    Serial.println(movingCount);

    Serial.print("Numar stari POSIBILA_CADERE: ");
    Serial.println(possibleFallCount);

    Serial.print("Numar stari IMPACT: ");
    Serial.println(impactCount);

    Serial.print("Numar stari CADERE_CONFIRMATA: ");
    Serial.println(confirmedFallCount);

    Serial.print("Cadere detectata in test: ");
    Serial.println(fallDetectedInTest ? "DA" : "NU");

    Serial.print("asvm minim: ");
    Serial.println(minAsvm, 3);

    Serial.print("asvm maxim: ");
    Serial.println(maxAsvm, 3);

    Serial.print("gsvm maxim: ");
    Serial.println(maxGsvm, 2);

    Serial.println("-------------------------------------");
    Serial.println();
}

// ===================== SETUP =====================
void setup()
{
    Serial.begin(115200);

    while (!Serial && millis() < 8000) {
        delay(10);
    }

    delay(1000);

    Serial.println("Pornire test IMU pentru validare functionala...");

    if (!IMU_init()) {
        Serial.println("EROARE: IMU nu a putut fi initializat.");
        while (true) {
            delay(1000);
        }
    }

    resetTestStats();
    printMenu();
}

// ===================== LOOP =====================
void loop()
{
    handleSerial();

    unsigned long now = millis();

    if (now - lastImuTime >= IMU_INTERVAL_MS) {
        lastImuTime = now;

        if (IMU_read(imuData)) {
            imuData = IMU_interpret(imuData);
            updateStats(imuData);
            printEventIfChanged(imuData);
        }
    }

    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
        lastPrintTime = now;
        printCsvLine(imuData);
    }

    if (now - lastSummaryTime >= SUMMARY_INTERVAL_MS) {
        lastSummaryTime = now;
        printSummary();
    }
}