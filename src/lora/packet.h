#ifndef PACKET_H
#define PACKET_H
#include <Arduino.h>
#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "alert/alert.h"
#include "fusion/fusion.h"
struct PacketData {
    char callSign[10];
    long counter;
    int battery;
    GpsData gpsData;
    AlertData alertData;
    LocationSource locationSource;
    uint8_t locationConfidence;


};

String packet_build(PacketData data);

#endif // PACKET_H