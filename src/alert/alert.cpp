#include "alert.h"

static bool pendingFall = false;


AlertData alert_evaluate(GpsData gpsData, IMUdata imuData, int battery)
{
    AlertData alertData;

    alertData.alertLevel = 0;
    alertData.eventType = EVENT_NORMAL;
    alertData.shouldTransmitNow = false;

    if (imuData.fallFlag) {
        pendingFall = true;
    }

    if (pendingFall && gpsData.valid) {
        alertData.alertLevel = 4;
        alertData.eventType = EVENT_FALL_DETECTED;
        alertData.shouldTransmitNow = true;
        return alertData;
    }

    if (pendingFall && !gpsData.valid) {
        alertData.alertLevel = 4;
        alertData.eventType = EVENT_FALL_NO_GPS;
        alertData.shouldTransmitNow = true;
        return alertData;
    }

    if (battery < 20) {
        alertData.alertLevel = 2;
        alertData.eventType = EVENT_LOW_BATTERY;
        alertData.shouldTransmitNow = true;
        return alertData;
    }

    return alertData;
}

void alert_clearPending()
{
    pendingFall = false;
}