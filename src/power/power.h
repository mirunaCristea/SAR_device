#ifndef POWER_H
#define POWER_H

// Structură utilizată pentru reprezentarea stării bateriei:
// tensiunea măsurată și procentul estimat de încărcare.
struct BatteryData {
    float voltage;
    int percent = 100;
};

// Inițializează citirea analogică folosită pentru monitorizarea bateriei.
void battery_init();

// Citește tensiunea bateriei și estimează nivelul de încărcare.
BatteryData battery_read();

#endif // POWER_H
