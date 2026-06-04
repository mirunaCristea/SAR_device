#include <Arduino.h>
#include "lora/lora_comm.h"
#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "lora/packet.h"
#include "audio/audio.h"
#include "alert/alert.h"


void setup()
{   
    Serial.begin(115200);
    delay(2000);
    lora_init();
    gps_init();
    IMU_init();
    // mic_init();

   
    // lora_init();
    delay(4000);
    
}

void loop()
{  
     static long counter = 0;
    static unsigned long lastSendTime = 0;

    GpsData gpsData = gps_read();
    IMUdata imuData = IMU_read();
    imuData=IMU_interpret(imuData);

    AlertData alertData = alert_evaluate(gpsData, imuData, 100);   // Placeholder pentru valoarea bateriei

    PacketData packetData;
    strcpy(packetData.callSign, CALL_SIGN);
    packetData.battery = 100; // Placeholder pentru valoarea bateriei
    packetData.gpsData = gpsData;
    packetData.alertData = alertData;


    if(millis() - lastSendTime > 5000) // Trimite un pachet la fiecare 5 secunde
    {
        lastSendTime = millis();
        counter++;
        packetData.counter = counter;
        String message = packet_build(packetData);
        Serial.println(message);
        lora_send(message.c_str());
        Serial.println(message.length());
        Serial.println("alertData: " + String(alertData.eventType) + ", " + String(alertData.alertLevel) + ", " + String(alertData.shouldTransmitNow));
        alert_clearPending(); // Resetează starea de alertă după trimiterea pachetului  
        // Construiește și trimite pachetul LoRa aici
    }
    



}