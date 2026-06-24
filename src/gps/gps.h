#ifndef GPS_H
#define GPS_H

// Structură utilizată pentru stocarea datelor extrase din mesajele GNSS.
struct GpsData
{
    float latitude = 0.0;
    float longitude = 0.0;
    float altitude = 0.0;

    // Indică dacă poziția curentă este considerată utilizabilă.
    bool valid = false;

    // Timp GPS în format "HH:MM:SS".
    char timestamp[12] = "00:00:00\0";

    int satCount = 0;
};

// Inițializează interfața serială folosită pentru comunicarea cu modulul GNSS.
void gps_init();

// Citește și parsează datele GNSS disponibile.
GpsData gps_read();

#endif // GPS_H