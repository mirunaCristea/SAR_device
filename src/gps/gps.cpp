#include <Arduino.h>
#include "gps.h"
#include "config.h"
#include <TinyGPS++.h>

TinyGPSPlus gps;
UART SerialGps(digitalPinToPinName(GPS_RX), digitalPinToPinName(GPS_TX), NC, NC);

void gps_init()
{
    SerialGps.begin(9600);
    Serial.println("GPS initializat");
}

GpsData gps_read()
{
    static GpsData data;
    data.valid = false;

    while (SerialGps.available() > 0) // returneaza nr de octeti valabili sa fie cititi din buffer
    {
        gps.encode(SerialGps.read()); // proceseaza un caracter primit de la GPS
        if (gps.location.isUpdated()) // daca s-a actualizat pozitia, actualizeaza structura GpsData
        {
            data.latitude = gps.location.lat();
            data.longitude = gps.location.lng();
            data.altitude = gps.altitude.meters();
            data.valid = true;
            data.satCount=gps.satellites.value(); // actualizeaza numarul de sateliti, chiar daca nu il folosim in structura GpsData
        }
        if (gps.time.isUpdated())
        {
            snprintf(data.timestamp, sizeof(data.timestamp), "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
        }
    }

    return data;
}

