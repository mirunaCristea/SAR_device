#ifndef PACKET_H
#define PACKET_H
#include <Arduino.h>
#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "alert/alert.h"
#include "fusion/fusion.h"
#include "power/power.h"
struct PacketData {
    char callSign[10];
    long counter;
    uint8_t batteryPercent;

    GpsData gpsData; // pt TIME
    AlertData alertData; // pt eventType si alertLevel
    FusionData fusionData; // pt pozitia finala + locationSource

};

String packet_build(PacketData data);

#endif // PACKET_H