#ifndef ALERT_H
#define ALERT_H
#include "imu/imu.h"
#include "gps/gps.h"

enum EventType {
    EVENT_NORMAL,
    EVENT_FALL_DETECTED,
    EVENT_FALL_NO_GPS,
    EVENT_LOW_BATTERY
};

struct AlertData {
    int alertLevel;
    EventType eventType;
    bool shouldTransmitNow;
};

AlertData alert_evaluate(GpsData gpsData, IMUdata imuData, int battery);
void alert_clearPending();

#endif // ALERT_H