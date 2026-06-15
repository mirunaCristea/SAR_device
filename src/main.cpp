#include <Arduino.h>

#include "alert/alert.h"
#include "audio/audio.h"
#include "config.h"
#include "fusion/fusion.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "lora/lora_comm.h"
#include "lora/packet.h"
#include "power/power.h"
#include "route/route.h"

// ===================== TIMING =====================

static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long SEND_INTERVAL_MS = 15000;
static const unsigned long SEND_RETRY_INTERVAL_MS = 5000;
static const unsigned long LORA_RETRY_INTERVAL_MS = 30000;
static const unsigned long BATTERY_READ_INTERVAL_MS = 15000;
static const unsigned long DEBUG_INTERVAL_MS = 1000;

// ===================== STATE =====================

static long packetCounter = 0;

static unsigned long lastImuTime = 0;
static unsigned long lastSendTime = 0;
static unsigned long lastSendAttemptTime = 0;
static unsigned long lastLoRaRetryTime = 0;
static unsigned long lastBatteryReadTime = 0;
static unsigned long lastDebugTime = 0;

static IMUdata imuData;
static AudioData audioData;
static BatteryData batteryData;
static RouteData routeData;

// ===================== INIT =====================

void setup()
{
    Serial.begin(115200);
    delay(5000);

    bool loraOk = lora_init();

    if (!loraOk) {
        Serial.println("Sistem pornit in mod degradat: LoRa indisponibil");
    }

    gps_init();
    IMU_init();
    audio_init();
    battery_init();

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    batteryData = battery_read();

    Serial.println("Sistem initializat");
}

// ===================== UPDATE FUNCTIONS =====================

static void update_imu(unsigned long now)
{
    if (now - lastImuTime < IMU_INTERVAL_MS) {
        return;
    }

    lastImuTime = now;

    if (IMU_read(imuData)) {
        imuData = IMU_interpret(imuData);
    }
}

static bool is_imu_critical_window()
{
    return imuData.imuState == IMU_POSSIBLE_FALL ||
           imuData.imuState == IMU_IMPACT;
}

static void update_audio()
{
    if (is_imu_critical_window()) {
        audio_reset_detector();
        return;
    }

    if (!audio_update(audioData)) {
        return;
    }

    if (audioData.state == AUDIO_HELP_DETECTED) {
        Serial.println();
        Serial.println(">>> ALERTA AUDIO HELP");

        Serial.print("Score=");
        Serial.print(audioData.helpScore);
        Serial.print("%");

        Serial.print(" | candidate=");
        Serial.print(audioData.helpCandidate ? "YES" : "NO");

        Serial.print(" | strong=");
        Serial.print(audioData.helpStrong ? "YES" : "NO");

        Serial.print(" | detector=");
        Serial.print(audioData.detectorCandidateCount);
        Serial.print("C/");
        Serial.print(audioData.detectorStrongCount);
        Serial.print("S/");
        Serial.print(audioData.detectorWindowCount);
        Serial.print("T");

        Serial.print(" | ratio=");
        Serial.print(audioData.detectorCandidateRatioPercent);
        Serial.println("%");
    }
}



static void update_battery(unsigned long now)
{
    if (now - lastBatteryReadTime < BATTERY_READ_INTERVAL_MS) {
        return;
    }

    lastBatteryReadTime = now;
    batteryData = battery_read();
}

static void retry_loRa_if_needed(unsigned long now)
{
    if (lora_isReady()) {
        return;
    }

    if (now - lastLoRaRetryTime < LORA_RETRY_INTERVAL_MS) {
        return;
    }

    lastLoRaRetryTime = now;
    lora_retryInit();
}

static void print_debug(
    unsigned long now,
    const FusionData& fusionData
)
{
    if (now - lastDebugTime < DEBUG_INTERVAL_MS) {
        return;
    }

    lastDebugTime = now;

    Serial.print("Fusion source=");
    Serial.print((int)fusionData.locationSource);

    Serial.print(" conf=");
    Serial.print(fusionData.locationConfidence);

    Serial.print(" valid=");
    Serial.print(fusionData.locationValid);

    Serial.print(" lat=");
    Serial.print(fusionData.latitude, 6);

    Serial.print(" lon=");
    Serial.print(fusionData.longitude, 6);

    Serial.print(" | Route=");
    Serial.print(route_statusToString(routeData.status));

    Serial.print(" dist=");
    Serial.print(routeData.distanceToRouteM);

    Serial.print(" m | Battery=");
    Serial.print(batteryData.percent);

    Serial.print("%");

    Serial.print(" | Audio=");
    Serial.print(audioData.state == AUDIO_HELP_DETECTED ? "HELP" : "NORMAL");

    Serial.print(" C=");
    Serial.print(audioData.detectorCandidateCount);

    Serial.print(" S=");
    Serial.print(audioData.detectorStrongCount);

    Serial.print(" T=");
    Serial.print(audioData.detectorWindowCount);

    Serial.print(" R=");
    Serial.print(audioData.detectorCandidateRatioPercent);
    Serial.println("%");
    }

static GpsData build_fused_GpsData(
    const GpsData& gpsData,
    const FusionData& fusionData
)
{
    GpsData fusedGpsData = gpsData;

    fusedGpsData.latitude = fusionData.latitude;
    fusedGpsData.longitude = fusionData.longitude;
    fusedGpsData.altitude = fusionData.altitude;
    fusedGpsData.valid = fusionData.locationValid;

    return fusedGpsData;
}

static bool should_try_send(
    unsigned long now,
    const AlertData& alertData
)
{
    bool periodicSend = now - lastSendTime >= SEND_INTERVAL_MS;
    bool emergencySend = alertData.shouldTransmitNow;

    if (!periodicSend && !emergencySend) {
        return false;
    }

    if (now - lastSendAttemptTime < SEND_RETRY_INTERVAL_MS) {
        return false;
    }

    return true;
}

static bool send_packet(
    const GpsData& gpsData,
    const FusionData& fusionData,
    const AlertData& alertData
)
{
    if (!lora_isReady()) {
        Serial.println("LoRa indisponibil. Alerta ramane activa local.");
        return false;
    }

    PacketData packetData;

    strcpy(packetData.callSign, CALL_SIGN);
    packetData.counter = ++packetCounter;
    packetData.batteryPercent = batteryData.percent;
    packetData.gpsData = gpsData;
    packetData.fusionData = fusionData;
    packetData.alertData = alertData;

    String message = packet_build(packetData);

    return lora_send(message.c_str());
}

// ===================== MAIN LOOP =====================

void loop()
{
    unsigned long now = millis();

    // 1. Citire senzori
    GpsData gpsData = gps_read();

    update_imu(now);
    update_audio();
    update_battery(now);

    // 2. Fuziune GPS-IMU
    FusionData fusionData = fusion_update(gpsData, imuData);

  

    // 3. Alertare
    GpsData fusedGpsData = build_fused_GpsData(gpsData, fusionData);

    AlertData alertData = alert_evaluate(
        fusedGpsData,
        imuData,
        audioData,
        batteryData.percent
    );

    // 4. Traseu
    bool criticalAlertActive =
    alertData.alertLevel >= 3 ||
    alertData.shouldTransmitNow;

    bool allowRouteBuzzer = !criticalAlertActive;

    routeData = route_update(fusionData, allowRouteBuzzer);


    // 5. Debug periodic
    print_debug(now, fusionData);

    // 6. Reiniţializare LoRa în caz de mod degradat
    retry_loRa_if_needed(now);

    // 7. Decizie transmitere
    if (!should_try_send(now, alertData)) {
        return;
    }

    lastSendAttemptTime = now;

    // 8. Construire şi transmitere pachet
    bool sentSuccessfully = send_packet(
        gpsData,
        fusionData,
        alertData
    );

    if (sentSuccessfully) {
        lastSendTime = now;

        if (alertData.shouldTransmitNow) {
            alert_clearPending();
        }
    }
}