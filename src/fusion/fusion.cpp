#include "fusion.h"
#include <math.h>
#include <Arduino.h>

namespace {

    const uint8_t GPS_CURRENT_CONFIDENCE = 90; //încredere cand gpsul este acceptat
    const uint8_t LAST_KNOWN_CONFIDENCE = 50; //încredere cand se foloseste ultima pozitie valida
    const uint8_t UNAVAILABLE_CONFIDENCE = 0; //încredere cand nu avem nicio pozitie valida
    const float STATIONARY_JUMP_THRESHOLD_M = 20; // prag de salt suspect pentru a respinge un GPS ca outlier
    const float MAX_REALISTIC_SPEED_MPS =  5; 
    const uint8_t REJECTED_GPS_CONFIDENCE = 35;
    const unsigned long MIN_SPEED_INTERVAL_MS = 1000;
    const float EARTH_RADIUS_M = 6371000.0f;


    float lastLatitude = 0.0f;
    float lastLongitude = 0.0f;
    float lastAltitude = 0.0f;
    bool lastPositionAvailable = false;
    unsigned long lastValidTimestamp = 0;

    float degrees_to_radians(float coord)
    {
        return coord * PI / 180.0;
    }

    float distance_meters(float lat1, float lon1, float lat2, float lon2) // prin Haversine
    {
        float lat1_rad = degrees_to_radians(lat1);
        float lat2_rad = degrees_to_radians(lat2);
        float lon1_rad = degrees_to_radians(lon1);
        float lon2_rad = degrees_to_radians(lon2);
        
        float delta_lat = lat2_rad - lat1_rad;
        float delta_lon = lon2_rad - lon1_rad;


        float a = sin(delta_lat / 2) * sin(delta_lat / 2) +
                  cos(lat1_rad) * cos(lat2_rad) *
                  sin(delta_lon / 2) * sin(delta_lon / 2);  // termenul Haversine

        a = constrain(a, 0.0f, 1.0f); // asigurăm că a este între 0 și 1 pentru a evita erorile de calcul din cauza impreciziei float-urilor
          
        float c = 2 * atan2(sqrt(a), sqrt(1 - a)); // unghiul central dintre cele doua puncte

        return EARTH_RADIUS_M * c; // distanta in metri


    }
}

FusionData fusion_update(const GpsData& gpsData, const IMUdata& imuData)
{
    FusionData fusionData;

    fusionData.motionState = imuData.motionState;
    fusionData.fallDetected = imuData.fallFlag;

    if (!gpsData.valid)
    {
        if (lastPositionAvailable)
        {
            fusionData.locationSource = LOCATION_LAST_KNOWN;
            fusionData.locationConfidence = LAST_KNOWN_CONFIDENCE;
            fusionData.latitude = lastLatitude;
            fusionData.longitude = lastLongitude;
            fusionData.altitude = lastAltitude;
            fusionData.locationValid = true;
        }
        else
        {
            fusionData.locationSource = LOCATION_UNAVAILABLE;
            fusionData.locationConfidence = UNAVAILABLE_CONFIDENCE;
            fusionData.latitude = 0.0f;
            fusionData.longitude = 0.0f;
            fusionData.altitude = 0.0f;
            fusionData.locationValid = false;
        }

        return fusionData;
    }

    if (!lastPositionAvailable)
    {
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

    if (elapsedMs < MIN_SPEED_INTERVAL_MS)
    {
        elapsedMs = MIN_SPEED_INTERVAL_MS;
    }

    float elapsedSec = elapsedMs / 1000.0f;
    float speed = displacement / elapsedSec;

    bool userIsStill =
        fusionData.motionState == STATIONARY ||
        fusionData.motionState == IMMOBILE;

    bool stationaryJump =
        userIsStill &&
        displacement > STATIONARY_JUMP_THRESHOLD_M;

    bool speedTooHigh =
        speed > MAX_REALISTIC_SPEED_MPS;

    bool rejectGps =
        stationaryJump ||
        speedTooHigh;

    if (rejectGps)
    {
        fusionData.locationSource = LOCATION_REJECTED_OUTLIER;
        fusionData.locationConfidence = REJECTED_GPS_CONFIDENCE;
        fusionData.latitude = lastLatitude;
        fusionData.longitude = lastLongitude;
        fusionData.altitude = lastAltitude;
        fusionData.locationValid = true;

        return fusionData;
    }

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