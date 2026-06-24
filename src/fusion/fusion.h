#ifndef FUSION_H
#define FUSION_H

#include "gps/gps.h"
#include "imu/imu.h"
#include <Arduino.h>

// Sursa poziției utilizate după etapa de validare GPS-IMU.
enum LocationSource
{
    LOCATION_UNAVAILABLE,
    LOCATION_GPS_CURRENT,
    LOCATION_LAST_KNOWN,
    LOCATION_REJECTED_OUTLIER
};

// Structură care reunește rezultatul fuziunii dintre datele GNSS și IMU.
struct FusionData {
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;

    // Indică dacă poziția transmisă mai departe este utilizabilă.
    bool locationValid = false;

    // Marchează proveniența poziției: GPS curent, ultima poziție validă sau poziție respinsă ca salt suspect.
    LocationSource locationSource = LOCATION_UNAVAILABLE;

    // Nivel estimativ de încredere asociat poziției rezultate.
    uint8_t locationConfidence = 0;

    // Starea de mișcare și rezultatul detecției de cădere sunt preluate din IMU.
    MotionState motionState = MOTION_UNKNOWN;
    bool fallDetected = false;
};

// Validează poziția GNSS folosind contextul oferit de datele inerțiale.
FusionData fusion_update(
    const GpsData& gpsData,
    const IMUdata& imuData
);

#endif // FUSION_H