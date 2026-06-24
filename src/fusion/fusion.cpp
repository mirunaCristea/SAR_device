#include "fusion.h"
#include <math.h>
#include <Arduino.h>

namespace {

    // Niveluri de încredere asociate sursei poziției utilizate.
    const uint8_t GPS_CURRENT_CONFIDENCE = 90;
    const uint8_t LAST_KNOWN_CONFIDENCE = 50;
    const uint8_t UNAVAILABLE_CONFIDENCE = 0;
    const uint8_t REJECTED_GPS_CONFIDENCE = 35;

    // Praguri pentru respingerea pozițiilor GPS instabile sau nerealiste.
    const float STATIONARY_JUMP_THRESHOLD_M = 20;
    const float MAX_REALISTIC_SPEED_MPS = 5;
    const unsigned long MIN_SPEED_INTERVAL_MS = 1000;

    const float EARTH_RADIUS_M = 6371000.0f;

    // Ultima poziție GPS acceptată este păstrată pentru fallback și comparații.
    float lastLatitude = 0.0f;
    float lastLongitude = 0.0f;
    float lastAltitude = 0.0f;
    bool lastPositionAvailable = false;
    unsigned long lastValidTimestamp = 0;

    float degrees_to_radians(float coord)
    {
        return coord * PI / 180.0;
    }

    // Calculează distanța dintre două coordonate geografice folosind formula Haversine.
    float distance_meters(float lat1, float lon1, float lat2, float lon2)
    {
        float lat1_rad = degrees_to_radians(lat1);
        float lat2_rad = degrees_to_radians(lat2);
        float lon1_rad = degrees_to_radians(lon1);
        float lon2_rad = degrees_to_radians(lon2);

        float delta_lat = lat2_rad - lat1_rad;
        float delta_lon = lon2_rad - lon1_rad;

        float a = sin(delta_lat / 2) * sin(delta_lat / 2) +
                  cos(lat1_rad) * cos(lat2_rad) *
                  sin(delta_lon / 2) * sin(delta_lon / 2);

        // Limitarea valorii reduce riscul unor erori numerice cauzate de float.
        a = constrain(a, 0.0f, 1.0f);

        float c = 2 * atan2(sqrt(a), sqrt(1 - a));

        return EARTH_RADIUS_M * c;
    }
}

FusionData fusion_update(const GpsData& gpsData, const IMUdata& imuData)
{
    FusionData fusionData;

    fusionData.motionState = imuData.motionState;
    fusionData.fallDetected = imuData.fallFlag;

    // Dacă GPS-ul nu este valid, se folosește ultima poziție acceptată dacă aceasta există.
    if (!gpsData.valid) {
        if (lastPositionAvailable) {
            fusionData.locationSource = LOCATION_LAST_KNOWN;
            fusionData.locationConfidence = LAST_KNOWN_CONFIDENCE;
            fusionData.latitude = lastLatitude;
            fusionData.longitude = lastLongitude;
            fusionData.altitude = lastAltitude;
            fusionData.locationValid = true;
        }
        else {
            fusionData.locationSource = LOCATION_UNAVAILABLE;
            fusionData.locationConfidence = UNAVAILABLE_CONFIDENCE;
            fusionData.latitude = 0.0f;
            fusionData.longitude = 0.0f;
            fusionData.altitude = 0.0f;
            fusionData.locationValid = false;
        }

        return fusionData;
    }

    // Prima poziție GPS validă este acceptată ca referință inițială.
    if (!lastPositionAvailable) {
        lastLatitude = gpsData.latitude;
        lastLongitude = gpsData.longitude;
        lastAltitude = gpsData.altitude;
        lastPositionAvailable = true;
        lastValidTimestamp = millis();

        fusionData.locationSource = LOCATION_GPS_CURRENT;
        fusionData.locationConfidence = GPS_CURRENT_CONFIDENCE;
        fusionData.latitude = gpsData.latitude;
        fusionData.longitude = gpsData.longitude;
        fusionData.altitude = gpsData.altitude;
        fusionData.locationValid = true;

        return fusionData;
    }

    float displacement = distance_meters(
        lastLatitude,
        lastLongitude,
        gpsData.latitude,
        gpsData.longitude
    );

    unsigned long elapsedMs = millis() - lastValidTimestamp;

    if (elapsedMs < MIN_SPEED_INTERVAL_MS) {
        elapsedMs = MIN_SPEED_INTERVAL_MS;
    }

    float elapsedSec = elapsedMs / 1000.0f;
    float speed = displacement / elapsedSec;

    bool userIsStill =
        fusionData.motionState == STATIONARY ||
        fusionData.motionState == IMMOBILE;

    // Poziția GPS este respinsă dacă indică un salt mare în staționare
    // sau o viteză aparentă nerealistă pentru scenariul analizat.
    bool stationaryJump =
        userIsStill &&
        displacement > STATIONARY_JUMP_THRESHOLD_M;

    bool speedTooHigh =
        speed > MAX_REALISTIC_SPEED_MPS;

    bool rejectGps =
        stationaryJump ||
        speedTooHigh;

    if (rejectGps) {
        fusionData.locationSource = LOCATION_REJECTED_OUTLIER;
        fusionData.locationConfidence = REJECTED_GPS_CONFIDENCE;
        fusionData.latitude = lastLatitude;
        fusionData.longitude = lastLongitude;
        fusionData.altitude = lastAltitude;
        fusionData.locationValid = true;

        return fusionData;
    }

    // Dacă poziția curentă trece verificările, aceasta devine noua referință validă.
    lastLatitude = gpsData.latitude;
    lastLongitude = gpsData.longitude;
    lastAltitude = gpsData.altitude;
    lastValidTimestamp = millis();

    fusionData.locationSource = LOCATION_GPS_CURRENT;
    fusionData.locationConfidence = GPS_CURRENT_CONFIDENCE;
    fusionData.latitude = gpsData.latitude;
    fusionData.longitude = gpsData.longitude;
    fusionData.altitude = gpsData.altitude;
    fusionData.locationValid = true;

    return fusionData;
}
