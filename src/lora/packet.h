#ifndef PACKET_H
#define PACKET_H

#include <Arduino.h>

#include "config.h"
#include "gps/gps.h"
#include "alert/alert.h"
#include "fusion/fusion.h"

// Aceasta reunește informațiile esențiale care trebuie transmise către beacon.
struct PacketData {
    char callSign[10];
    long counter;
    uint8_t batteryPercent;

    // Timpul GPS este preluat din structura GpsData.
    GpsData gpsData;

    // Nivelul alertei și tipul evenimentului sunt preluate din AlertData.
    AlertData alertData;

    // Poziția finală validată și sursa acesteia sunt preluate din FusionData.
    FusionData fusionData;
};

String packet_build(PacketData data);

#endif // PACKET_H

