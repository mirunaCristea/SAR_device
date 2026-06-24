#include "alert.h"

#include <Arduino.h>

static const unsigned long FALL_MEMORY_MS = 3UL * 60UL * 1000UL;
static const unsigned long HELP_MEMORY_MS = 60UL * 1000UL;

static bool fallMemoryActive = false;
static bool helpMemoryActive = false;
static bool pendingLowBattery = false;

static bool fallSent = false;
static bool helpSent = false;
static bool fallAndHelpSent = false;

static unsigned long lastFallTime = 0;
static unsigned long lastHelpTime = 0;

static bool helpWasActive = false;
static bool lowBatteryWasActive = false;

/*
fall + help     -> nivel 4
fall            -> nivel 4
help            -> nivel 3
low battery     -> nivel 2
normal          -> nivel 0
*/

static bool is_recent(unsigned long now, unsigned long eventTime, unsigned long windowMs)
{
    return now - eventTime <= windowMs;
}

static void update_event_memory(
    unsigned long now,
    const IMUdata& imuData,
    const AudioData& audioData,
    int battery
)
{
    if (imuData.fallFlag) {
        fallMemoryActive = true;
        lastFallTime = now;
        fallSent = false;
        fallAndHelpSent = false;
    }

    bool helpDetected =
        audioData.valid &&
        audioData.state == AUDIO_HELP_DETECTED;

    if (helpDetected && !helpWasActive) {
        helpMemoryActive = true;
        lastHelpTime = now;
        helpSent = false;
        fallAndHelpSent = false;
    }

    helpWasActive = helpDetected;

    bool lowBattery = battery < 20;

    if (lowBattery && !lowBatteryWasActive) {
        pendingLowBattery = true;
    }

    lowBatteryWasActive = lowBattery;
}

static void expire_old_events(unsigned long now)
{
    if (fallMemoryActive && !is_recent(now, lastFallTime, FALL_MEMORY_MS)) {
        fallMemoryActive = false;
        fallSent = false;
        fallAndHelpSent = false;
    }

    if (helpMemoryActive && !is_recent(now, lastHelpTime, HELP_MEMORY_MS)) {
        helpMemoryActive = false;
        helpSent = false;
        fallAndHelpSent = false;
    }
}

AlertData alert_evaluate(GpsData gpsData, IMUdata imuData, AudioData audioData, int battery)
{
    AlertData alertData;

    alertData.alertLevel = 0;
    alertData.eventType = EVENT_NORMAL;
    alertData.shouldTransmitNow = false;

    unsigned long now = millis();

    update_event_memory(now, imuData, audioData, battery);
    expire_old_events(now);

    bool recentFall =
        fallMemoryActive &&
        is_recent(now, lastFallTime, FALL_MEMORY_MS);

    bool recentHelp =
        helpMemoryActive &&
        is_recent(now, lastHelpTime, HELP_MEMORY_MS);

    bool helpDetectedAfterFall =
        recentFall &&
        recentHelp &&
        lastHelpTime >= lastFallTime;

    if (helpDetectedAfterFall && fallSent && !fallAndHelpSent) {
        alertData.alertLevel = 4;
        alertData.eventType = EVENT_FALL_AND_AUDIO_DISTRESS;
        alertData.shouldTransmitNow = true;
        return alertData;
    }

    if (recentFall && !fallSent) {
        alertData.alertLevel = 4;
        alertData.eventType =
            gpsData.valid ? EVENT_FALL_DETECTED : EVENT_FALL_NO_GPS;
        alertData.shouldTransmitNow = true;
        return alertData;
    }

    if (recentHelp && !helpSent) {
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

void alert_markSent(EventType eventType)
{
    switch (eventType) {
        case EVENT_FALL_DETECTED:
        case EVENT_FALL_NO_GPS:
            fallSent = true;
            break;

        case EVENT_AUDIO_DISTRESS:
            helpSent = true;
            break;

        case EVENT_FALL_AND_AUDIO_DISTRESS:
            fallSent = true;
            helpSent = true;
            fallAndHelpSent = true;
            break;

        case EVENT_LOW_BATTERY:
            pendingLowBattery = false;
            break;

        case EVENT_NORMAL:
        default:
            break;
    }
}
