#ifndef ROUTE_H
#define ROUTE_H

#include "fusion/fusion.h"

// Stările posibile ale utilizatorului față de traseul de referință.
enum RouteStatus {
    ROUTE_UNKNOWN,
    ROUTE_ON_ROUTE,
    ROUTE_NEAR_LIMIT,
    ROUTE_OFF_ROUTE
};

// Structură folosită pentru transmiterea rezultatului evaluării traseului
// către bucla principală și către mesajele de diagnostic.
struct RouteData {
    RouteStatus status;
    float distanceToRouteM;
    bool warningActive;
};

// Evaluează poziția fuzionată față de traseul definit și decide dacă
// avertizarea locală prin buzzer poate fi activată.
RouteData route_update(
    const FusionData& fusionData,
    bool allowLocalBuzzer
);

// Conversie a stării traseului în text, utilă pentru afișare și debug serial.
const char* route_statusToString(RouteStatus status);

#endif