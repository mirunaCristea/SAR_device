#include <Arduino.h>

#include "alert/alert.h"
#include "audio/audio.h"
#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "lora/lora_comm.h"
#include "lora/packet.h"

static const unsigned long IMU_INTERVAL_MS = 20;
static const unsigned long IMU_PRINT_INTERVAL_MS = 2000;
static const unsigned long FALL_DISPLAY_DURATION_MS = 5000;
static const unsigned long SEND_INTERVAL_MS = 5000;

static const char *motionStateName(MotionState state)
{
    switch (state) {
        case MOVING: return "IN MISCARE";
        case STATIONARY: return "STATIONAR";
        case IMMOBILE: return "IMOBIL";
        default: return "NECUNOSCUT";
    }
}

static const char *imuStateName(ImuState state)
{
    switch (state) {
        case IMU_IMPACT: return "IMPACT";
        case IMU_POSSIBLE_FALL: return "CADERE POSIBILA";
        case IMU_CONFIRMED_FALL: return "CADERE CONFIRMATA";
        default: return "NORMAL";
    }
}

void setup()
{
    Serial.begin(115200);
    delay(5000);

    lora_init();
    gps_init();
    IMU_init();
    audio_init();
}

void loop()
{
    static long counter = 0;
    static unsigned long lastImuTime = 0;
    static unsigned long lastImuPrintTime = 0;
    static unsigned long lastFallDetectedTime = 0;
    static unsigned long lastSendTime = 0;
    static IMUdata imuData;
    static AudioData audioData;

    GpsData gpsData = gps_read();
    unsigned long now = millis();

    if (now - lastImuTime >= IMU_INTERVAL_MS) {
        lastImuTime = now;
        imuData = IMU_interpret(IMU_read());

        if (imuData.fallFlag) {
            lastFallDetectedTime = now;
        }
    }

    if (now - lastImuPrintTime >= IMU_PRINT_INTERVAL_MS) {
        lastImuPrintTime = now;
        bool recentFall =
            lastFallDetectedTime != 0 &&
            now - lastFallDetectedTime < FALL_DISPLAY_DURATION_MS;

        // Serial.println();
        // Serial.println("===== STARE IMU =====");
        // Serial.print("Acceleratie [g]   X: ");
        // Serial.print(imuData.ax, 3);
        // Serial.print("  Y: ");
        // Serial.print(imuData.ay, 3);
        // Serial.print("  Z: ");
        // Serial.println(imuData.az, 3);
        // Serial.print("Modul acceleratie: ");
        // Serial.print(imuData.asvm, 3);
        // Serial.println(" g");
        // Serial.print("Modul giroscop:    ");
        // Serial.print(imuData.gsvm, 2);
        // Serial.println(" dps");
        // Serial.print("Stare miscare:     ");
        // Serial.println(motionStateName(imuData.motionState));
        // Serial.print("Stare detectie:    ");
        // Serial.println(
        //     recentFall ? "CADERE CONFIRMATA" : imuStateName(imuData.imuState)
        // );
        // Serial.print("Indicator cadere:  ");
        // Serial.println(recentFall ? "DA" : "NU");
        // Serial.println("======================");
    }

    audio_update(audioData);
//     if (audio_update(audioData)) {
//     // Serial.print("Audio: ");
//     // Serial.print(audioData.state == AUDIO_HELP_DETECTED ? "HELP" : "NORMAL");
//     // Serial.print(" | score=");
//     // Serial.println(audioData.helpScore);
//    }
    

    AlertData alertData =
        alert_evaluate(gpsData, imuData, audioData, 100);

    if (now - lastSendTime < SEND_INTERVAL_MS) {
        return;
    }

    lastSendTime = now;

    PacketData packetData;
    strcpy(packetData.callSign, CALL_SIGN);
    packetData.counter = ++counter;
    packetData.battery = 100;
    packetData.gpsData = gpsData;
    packetData.alertData = alertData;

    String message = packet_build(packetData);
    //lora_send(message.c_str());
    alert_clearPending();
}
