#include <Arduino.h>
#include "gps.h"
#include "config.h"
#include <TinyGPS++.h>

TinyGPSPlus gps;

// Interfață UART configurată pe pinii aleși pentru modulul GNSS.
UART SerialGps(
    digitalPinToPinName(GPS_RX),
    digitalPinToPinName(GPS_TX),
    NC,
    NC
);

// Poziția este considerată recentă doar pentru un interval limitat pentru a evita utilizarea unor coordonate învechite.
static const unsigned long GPS_POSITION_MAX_AGE_MS = 5000;

void gps_init()
{
    SerialGps.begin(9600);
    Serial.println("GPS initializat");
}

GpsData gps_read()
{
    static GpsData data;

    // Se citesc toate caracterele disponibile din fluxul NMEA.
    while (SerialGps.available() > 0) {
        gps.encode(SerialGps.read());

        // isUpdated indică apariția unei valori noi, nu validitatea finală a poziției.
        if (gps.location.isUpdated()) {
            data.latitude = gps.location.lat();
            data.longitude = gps.location.lng();
            data.altitude = gps.altitude.meters();
        }

        if (gps.satellites.isUpdated()) {
            data.satCount = gps.satellites.value();
        }

        if (gps.time.isUpdated()) {
            snprintf(
                data.timestamp,
                sizeof(data.timestamp),
                "%02d:%02d:%02d",
                gps.time.hour(),
                gps.time.minute(),
                gps.time.second()
            );
        }
    }

    // Validarea poziției ține cont atât de statusul GNSS, cât și de vechimea ultimei poziții primite.
    data.valid =
        gps.location.isValid() &&
        gps.location.age() <= GPS_POSITION_MAX_AGE_MS;

    return data;
}