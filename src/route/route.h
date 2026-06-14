#ifndef ROUTE_H
#define ROUTE_H

#include "fusion/fusion.h"

enum RouteStatus {
    ROUTE_UNKNOWN,
    ROUTE_ON_ROUTE,
    ROUTE_NEAR_LIMIT,
    ROUTE_OFF_ROUTE
};

struct RouteData {
    RouteStatus status;
    float distanceToRouteM;
    bool warningActive;
};

RouteData route_update(const FusionData& fusionData, bool allowLocalBuzzer );
const char* route_statusToString(RouteStatus status);

#endif