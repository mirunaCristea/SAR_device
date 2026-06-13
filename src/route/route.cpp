#include "route.h"
#include "route_config.h"
#include "config.h"
#include <Arduino.h>
#include <math.h>

static const float EARTH_RADIUS_M = 6371000.0f;

static const float OFF_ROUTE_THRESHOLD_M = 30.0f;
static const float BACK_ON_ROUTE_THRESHOLD_M = 20.0f;
static const int REQUIRED_OFF_ROUTE_COUNT = 3;

static int offRouteCounter = 0;
static bool warningActive = false;
static bool buzzerAlreadyPlayed = false;

static float degToRad(float deg)
{
    return deg * PI / 180.0f;
}

static float clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void buzzThreeTimes()
{
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);

        digitalWrite(BUZZER_PIN, LOW);
        delay(250);
    }
}

static void latLonToXY(
    float originLat,
    float originLon,
    float lat,
    float lon,
    float& x,
    float& y
)
{
    float originLatRad = degToRad(originLat);

    float dLat = degToRad(lat - originLat);
    float dLon = degToRad(lon - originLon);

    x = EARTH_RADIUS_M * dLon * cos(originLatRad);
    y = EARTH_RADIUS_M * dLat;
}

static float distancePointToSegment(
    float pLat,
    float pLon,
    RoutePoint a,
    RoutePoint b
)
{
    float px, py;
    float bx, by;

    latLonToXY(a.lat, a.lon, pLat, pLon, px, py);
    latLonToXY(a.lat, a.lon, b.lat, b.lon, bx, by);

    float segmentLen2 = bx * bx + by * by;

    if (segmentLen2 < 0.0001f) {
        return sqrt(px * px + py * py);
    }

    float t = (px * bx + py * by) / segmentLen2;
    t = clamp01(t);

    float closestX = t * bx;
    float closestY = t * by;

    float dx = px - closestX;
    float dy = py - closestY;

    return sqrt(dx * dx + dy * dy);
}

static float distanceToRoute(float lat, float lon)
{
    float minDistance = 999999.0f;

    for (int i = 0; i < ROUTE_POINT_COUNT - 1; i++) {
        float d = distancePointToSegment(
            lat,
            lon,
            ROUTE_POINTS[i],
            ROUTE_POINTS[i + 1]
        );

        if (d < minDistance) {
            minDistance = d;
        }
    }

    return minDistance;
}

RouteData route_update(const FusionData& fusionData)
{
    RouteData data;

    data.status = ROUTE_UNKNOWN;
    data.distanceToRouteM = 0.0f;
    data.warningActive = false;

    if (!fusionData.locationValid) {
        return data;
    }

    if (fusionData.locationSource != LOCATION_GPS_CURRENT) {
        return data;
    }

    float distance = distanceToRoute(
        fusionData.latitude,
        fusionData.longitude
    );

    data.distanceToRouteM = distance;

    if (distance > OFF_ROUTE_THRESHOLD_M) {
        offRouteCounter++;

        if (offRouteCounter >= REQUIRED_OFF_ROUTE_COUNT) {
            warningActive = true;
        }
    }
    else if (distance < BACK_ON_ROUTE_THRESHOLD_M) {
        offRouteCounter = 0;
        warningActive = false;
        buzzerAlreadyPlayed = false;
    }

    if (warningActive && !buzzerAlreadyPlayed) {
        buzzThreeTimes();
        buzzerAlreadyPlayed = true;
    }

    data.warningActive = warningActive;

    if (warningActive) {
        data.status = ROUTE_OFF_ROUTE;
    }
    else if (distance >= BACK_ON_ROUTE_THRESHOLD_M &&
             distance <= OFF_ROUTE_THRESHOLD_M) {
        data.status = ROUTE_NEAR_LIMIT;
    }
    else {
        data.status = ROUTE_ON_ROUTE;
    }

    return data;
}

const char* route_statusToString(RouteStatus status)
{
    switch (status) {
        case ROUTE_ON_ROUTE:
            return "ON_ROUTE";

        case ROUTE_NEAR_LIMIT:
            return "NEAR_LIMIT";

        case ROUTE_OFF_ROUTE:
            return "OFF_ROUTE";

        case ROUTE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}