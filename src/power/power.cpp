#include "power.h"
#include <Arduino.h>

#define BATTERY_PIN A1

static const float ADC_REF_V = 3.19f;
static const int ADC_BITS = 12;
static const int ADC_MAX = (1 << ADC_BITS) - 1;

// Schema ta: R18 = 4.7k, R17 = 4.7k
static const float R_TOP = 4700.0f;
static const float R_BOTTOM = 4700.0f;

void battery_init()
{
    analogReadResolution(ADC_BITS);
}

static float read_battery_voltage()
{
    const int samples = 20;
    long sum = 0;

    for (int i = 0; i < samples; i++) {
        sum += analogRead(BATTERY_PIN);
        delay(2);
    }

    float raw = sum / (float)samples;

    float v_adc = raw * ADC_REF_V / ADC_MAX;

    // Reconstruiește tensiunea reală a bateriei
    float v_bat = v_adc * (R_TOP + R_BOTTOM) / R_BOTTOM;

    return v_bat;
}

static int estimate_battery_percent(float vbat)
{
    if (vbat >= 4.20f) return 100;
    if (vbat >= 4.10f) return 90;
    if (vbat >= 4.00f) return 80;
    if (vbat >= 3.90f) return 70;
    if (vbat >= 3.80f) return 55;
    if (vbat >= 3.70f) return 40;
    if (vbat >= 3.60f) return 25;
    if (vbat >= 3.50f) return 15;
    if (vbat >= 3.40f) return 8;
    if (vbat >= 3.30f) return 3;

    return 0;
}

BatteryData battery_read()
{
    BatteryData data;

    data.voltage = read_battery_voltage();
    data.percent = estimate_battery_percent(data.voltage);

    return data;
}