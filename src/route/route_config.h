#ifndef ROUTE_CONFIG_H
#define ROUTE_CONFIG_H

struct RoutePoint {
    float lat;
    float lon;
};

static const RoutePoint ROUTE_POINTS[] = {
    {44.435000f, 26.047000f},
    {44.435200f, 26.047500f},
    {44.435500f, 26.048000f},
    {44.435900f, 26.048500f}
};

static const int ROUTE_POINT_COUNT =
    sizeof(ROUTE_POINTS) / sizeof(ROUTE_POINTS[0]);

#endif