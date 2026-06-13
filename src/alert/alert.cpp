#include "alert.h"

static bool pendingFall = false;
static bool pendingHelp = false;
static bool pendingLowBattery = false;

static bool helpWasActive = false;
static bool lowBatteryWasActive = false;

/*
fall + help     → nivel 4
fall            → nivel 4
help            → nivel 3
low battery     → nivel 2
normal          → nivel 0
*/


AlertData alert_evaluate(GpsData gpsData, IMUdata imuData, AudioData audioData, int battery)
{
    AlertData alertData;

    alertData.alertLevel = 0;
    alertData.eventType = EVENT_NORMAL;
    alertData.shouldTransmitNow = false;

  

    if (imuData.fallFlag) {
        pendingFall = true;
    }

    bool helpDetected =
    audioData.valid &&
    audioData.state == AUDIO_HELP_DETECTED;

    if (helpDetected && !helpWasActive) {
    pendingHelp = true;
    }

    helpWasActive = helpDetected;

    bool lowBattery = battery < 20;

    if (lowBattery && !lowBatteryWasActive) {
        pendingLowBattery = true;
    }

    lowBatteryWasActive = lowBattery;


    if (pendingFall && pendingHelp) {
    alertData.alertLevel = 4;
    alertData.eventType = EVENT_FALL_AND_AUDIO_DISTRESS;
    alertData.shouldTransmitNow = true;
    return alertData;
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


    if (pendingHelp) {
        alertData.alertLevel = 3;
        alertData.eventType = EVENT_AUDIO_DISTRESS;
        alertData.shouldTransmitNow = true;
        return alertData;
    }

    if (pendingLowBattery) {
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
    pendingHelp = false;
    pendingLowBattery = false;
}
