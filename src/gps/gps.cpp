#include <Arduino.h>
#include "gps.h"
#include "config.h"
#include <TinyGPS++.h>

TinyGPSPlus gps;
UART SerialGps(digitalPinToPinName(GPS_RX), digitalPinToPinName(GPS_TX), NC, NC);

static const unsigned long GPS_POSITION_MAX_AGE_MS = 5000; // pozitie considerata recenta timp de 5 s

void gps_init()
{
    SerialGps.begin(9600);
    Serial.println("GPS initializat");
}

GpsData gps_read()
{
    static GpsData data;

    while (SerialGps.available() > 0)
    {
        gps.encode(SerialGps.read());

        // isUpdated inseamna doar ca a venit o pozitie noua acum.
        // Nu il folosim ca definitie finala pentru valid.
        if (gps.location.isUpdated())
        {
            data.latitude = gps.location.lat();
            data.longitude = gps.location.lng();
            data.altitude = gps.altitude.meters();
        }

        if (gps.satellites.isUpdated())
        {
            data.satCount = gps.satellites.value();
        }

        if (gps.time.isUpdated())
        {
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

    // Aici decidem daca pozitia GPS este utilizabila.
    // Pozitia ramane valida cateva secunde dupa ultimul update.
    data.valid =
        gps.location.isValid() &&
        gps.location.age() <= GPS_POSITION_MAX_AGE_MS;

    return data;
}