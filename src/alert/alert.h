#ifndef ALERT_H
#define ALERT_H

#include <stdint.h>

#include "imu/imu.h"
#include "gps/gps.h"
#include "audio/audio.h"

// Tipurile de evenimente care pot fi raportate de sistem.
enum EventType {
    EVENT_NORMAL,
    EVENT_FALL_DETECTED,
    EVENT_FALL_NO_GPS,
    EVENT_LOW_BATTERY,
    EVENT_AUDIO_DISTRESS,
    EVENT_FALL_AND_AUDIO_DISTRESS,
};

// Structură utilizată pentru transmiterea rezultatului logicii de alertare
// către bucla principală și către modulul de pachetizare LoRa.
struct AlertData {
    uint8_t alertLevel;
    EventType eventType;
    bool shouldTransmitNow;
};

// Evaluează datele senzoriale și stabilește nivelul de alertă curent.
AlertData alert_evaluate(
    GpsData gpsData,
    IMUdata imuData,
    AudioData audioData,
    int battery
);

// Marchează un eveniment ca transmis, pentru a evita retrimiterea continuă
// a aceleiași alerte.
void alert_markSent(EventType eventType);

#endif // ALERT_H