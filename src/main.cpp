#include <Arduino.h>

#include "alert/alert.h"
#include "audio/audio.h"
#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "lora/lora_comm.h"
#include "lora/packet.h"

static const unsigned long IMU_INTERVAL_MS = 20;
static const unsigned long SEND_INTERVAL_MS = 5000;

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
    static unsigned long lastSendTime = 0;
    static IMUdata imuData;
    static AudioData audioData;

    GpsData gpsData = gps_read();
    unsigned long now = millis();

    if (now - lastImuTime >= IMU_INTERVAL_MS) {
        lastImuTime = now;
        imuData = IMU_interpret(IMU_read());
    }

    if (audio_update(audioData)) {
    Serial.print("Audio: ");
    Serial.print(audioData.state == AUDIO_HELP_DETECTED ? "HELP" : "NORMAL");
    Serial.print(" | score=");
    Serial.println(audioData.helpScore);
   }
    

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
    lora_send(message.c_str());
    alert_clearPending();
}
