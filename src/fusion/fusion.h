#ifndef FUSION_H
#define FUSION_H

#include "gps/gps.h"
#include "imu/imu.h"
#include <Arduino.h>

enum LocationSource
{
    LOCATION_UNAVAILABLE, //nu există nicio poziție utilizabilă
    LOCATION_GPS_CURRENT, // poziția GPS curentă a fost acceptată
    LOCATION_LAST_KNOWN, //poziția GPS lipseste, dar se foloseste ultima pozitie valida
    LOCATION_REJECTED_OUTLIER //GPS-ul curent a fost respins ca salt suspect
};

struct FusionData {
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;

    bool locationValid = false;
    LocationSource locationSource = LOCATION_UNAVAILABLE;
    uint8_t locationConfidence = 0;

    MotionState motionState = MOTION_UNKNOWN;
    bool fallDetected = false;
};

FusionData fusion_update(const GpsData& gpsData, const IMUdata& imuData);
#endif // FUSION_H
