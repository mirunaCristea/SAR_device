#ifndef ALERT_H
#define ALERT_H

#include "imu/imu.h"
#include "gps/gps.h"
#include "audio/audio.h"

enum EventType {
    EVENT_NORMAL,
    EVENT_FALL_DETECTED,
    EVENT_FALL_NO_GPS,
    EVENT_LOW_BATTERY,
    EVENT_AUDIO_DISTRESS,
    EVENT_FALL_AND_AUDIO_DISTRESS,
};

struct AlertData {
    uint8_t alertLevel;
    EventType eventType;
    bool shouldTransmitNow;
};

AlertData alert_evaluate(GpsData gpsData, IMUdata imuData, AudioData audioData, int battery);
void alert_clearPending();

#endif // ALERT_H