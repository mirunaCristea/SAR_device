#ifndef FUSION_H
#define FUSION_H

#include "gps/gps.h"
#include "imu/imu.h"

enum LocationSource
{
    LOCATION_UNAVAILABLE,
    LOCATION_GPS_,
    LOCATION_LAST_KNOWN
};

#endif // FUSION_H
