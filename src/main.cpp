#include <Arduino.h>

#include "alert/alert.h"
#include "audio/audio.h"
#include "config.h"
#include "fusion/fusion.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "lora/lora_comm.h"
#include "lora/packet.h"

static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long SEND_INTERVAL_MS = 15000;



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
    static unsigned long lastFusionDebugTime = 0;
    static long counter = 0;
    static unsigned long lastImuTime = 0;
    static unsigned long lastSendTime = 0;
    static IMUdata imuData;
    static AudioData audioData;

    GpsData gpsData = gps_read();
    unsigned long now = millis();

    if (now - lastImuTime >= IMU_INTERVAL_MS) {
        lastImuTime = now;

        if (IMU_read(imuData))
        {
            imuData = IMU_interpret(imuData);
        }
    }

    bool imuWindowActive =
    imuData.imuState == IMU_POSSIBLE_FALL ||
    imuData.imuState == IMU_IMPACT;

    if (!imuWindowActive && audio_update(audioData)) {
        Serial.print("Audio: ");
        Serial.print(
            audioData.state == AUDIO_HELP_DETECTED ? "HELP" : "NORMAL"
        );
        Serial.print(" | score=");
        Serial.println(audioData.helpScore);
    }
    
    FusionData fusionData = fusion_update(gpsData, imuData);
    GpsData fusedGpsData=gpsData;
    fusedGpsData.latitude = fusionData.latitude;
    fusedGpsData.longitude = fusionData.longitude;
    fusedGpsData.altitude = fusionData.altitude;
    fusedGpsData.valid = fusionData.locationValid;
    AlertData alertData = alert_evaluate(fusedGpsData, imuData, audioData, 100);

    if (now - lastFusionDebugTime >= 1000) {
    lastFusionDebugTime = now;

    Serial.print("Fusion source=");
    Serial.print((int)fusionData.locationSource);
    Serial.print(" conf=");
    Serial.print(fusionData.locationConfidence);
    Serial.print(" valid=");
    Serial.print(fusionData.locationValid);
    Serial.print(" lat=");
    Serial.print(fusionData.latitude, 6);
    Serial.print(" lon=");
    Serial.println(fusionData.longitude, 6);
}

    bool periodicSend =
        now - lastSendTime >= SEND_INTERVAL_MS;

    bool emergencySend =
        alertData.shouldTransmitNow;

    if (!periodicSend && !emergencySend) {
        return;
    }

   

    PacketData packetData;
    strcpy(packetData.callSign, CALL_SIGN);
    packetData.counter = ++counter;
    packetData.battery = 100;
    packetData.gpsData = fusedGpsData;
    packetData.alertData = alertData;
    packetData.locationSource = fusionData.locationSource;
    packetData.locationConfidence = fusionData.locationConfidence;

    String message = packet_build(packetData);
    bool sentSuccessfully = lora_send(message.c_str());

    if (sentSuccessfully) {
        lastSendTime = now;

        if (emergencySend) {
            alert_clearPending();
        }
    }


}
