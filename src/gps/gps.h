#ifndef GPS_H
#define GPS_H

struct GpsData
{
    float latitude=0.0;
    float longitude=0.0;
    float altitude=0.0;
    bool valid=false;
    char timestamp[12]="00:00:00\0"; // "HH:MM:SS\0"
    int satCount=0;
};


void gps_init();
GpsData gps_read();
#endif // GPS_H