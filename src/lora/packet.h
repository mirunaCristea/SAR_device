#ifndef PACKET_H
#define PACKET_H
#include <Arduino.h>
#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "alert/alert.h"

struct PacketData {
    char callSign[10];
    long counter;
    int battery;
    GpsData gpsData;
    AlertData alertData;

};

String packet_build(PacketData data);

#endif // PACKET_H