#ifndef POWER_H
#define POWER_H

struct BatteryData {
    float voltage;
    int percent = 100;
};

void battery_init();
BatteryData battery_read();



#endif // POWER_H