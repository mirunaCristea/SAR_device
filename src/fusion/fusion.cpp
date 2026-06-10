#include "fusion.h"
#include <math.h>
#include <Arduino.h>

namespace {

    const uint8_t GPS_CURRENT_CONFIDENCE = 90; //încredere cand gpsul este acceptat
    const uint8_t LAST_KNOWN_CONFIDENCE = 50; //încredere cand se foloseste ultima pozitie valida
    const uint8_t UNAVAILABLE_CONFIDENCE = 0; //încredere cand nu avem nicio pozitie valida
    const uint8_t STATIONARY_JUMP_THRESHOLD = 20; // prag de salt suspect pentru a respinge un GPS ca outlier
    const uint8_t MAX_REALISTIC_SPEED_MPS =  5; 


    float lastLatitude = 0.0f;
    float lastLongitude = 0.0f;
    float lastAltitude = 0.0f;
    bool lastPositionAvailable = false;
    unsigned long lastValidTimestamp = 0;

    long grades_radians(float coord)
    {
        return coord * PI / 180.0;
    }

    unsigned long distance_meters(float lat1, float lon1, float lat2, float lon2) // prin Haversine
    {
        float R = 6371000; // raza pamantului in metri
        float lat1_rad = grades_radians(lat1);
        float lat2_rad = grades_radians(lat2);
        float lon1_rad = grades_radians(lon1);
        float lon2_rad = grades_radians(lon2);
        
        float delta_lat = lat2_rad - lat1_rad;
        float delta_lon = lon2_rad - lon1_rad;

        float a = sin(delta_lat / 2) * sin(delta_lat / 2) +
                  cos(lat1_rad) * cos(lat2_rad) *
                  sin(delta_lon / 2) * sin(delta_lon / 2);  // termenul Haversine

        float c = 2 * atan2(sqrt(a), sqrt(1 - a)); // unghiul central dintre cele doua puncte

        return R * c; // distanta in metri


    }
}

FusionData fusion_update(const GpsData& gpsData, const IMUdata& imuData)
{   FusionData fusionData;
    fusionData.motionState = imuData.motionState;
    fusionData.fallDetected = imuData.fallFlag;
    if (gpsData.valid)
    {
        lastLatitude = gpsData.latitude;
        lastLongitude = gpsData.longitude;
        lastAltitude = gpsData.altitude;
        lastPositionAvailable = true;
        lastValidTimestamp = millis();

        fusionData.locationSource= LOCATION_GPS_CURRENT;
        fusionData.locationConfidence = GPS_CURRENT_CONFIDENCE;
        fusionData.latitude = gpsData.latitude;
        fusionData.longitude = gpsData.longitude;
        fusionData.altitude = gpsData.altitude;
        fusionData.locationValid = true;
    
    }
    else
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
    }
 return fusionData;

}