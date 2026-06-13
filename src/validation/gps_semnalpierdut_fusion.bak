#include <Arduino.h>

#include "gps/gps.h"
#include "imu/imu.h"
#include "fusion/fusion.h"

static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long PRINT_INTERVAL_MS = 1000;

static unsigned long startTime = 0;

const char* motionStateToText(MotionState state)
{
    switch (state)
    {
        case MOVING:
            return "MISCARE";
        case STATIONARY:
            return "STATIONAR";
        case IMMOBILE:
            return "IMOBIL";
        default:
            return "NECUNOSCUT";
    }
}

const char* locationSourceToText(LocationSource source)
{
    switch (source)
    {
        case LOCATION_UNAVAILABLE:
            return "POZITIE INDISPONIBILA";
        case LOCATION_GPS_CURRENT:
            return "GPS CURENT ACCEPTAT";
        case LOCATION_LAST_KNOWN:
            return "ULTIMA POZITIE VALIDA";
        case LOCATION_REJECTED_OUTLIER:
            return "GPS RESPINS - SALT SUSPECT";
        default:
            return "SURSA NECUNOSCUTA";
    }
}

void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("==================================================");
    Serial.println(" TEST 3 - VALIDARE GPS STATIONAR + FUZIUNE GPS-IMU");
    Serial.println("==================================================");
    Serial.println();
    Serial.println("Obiectiv:");
    Serial.println("- verificarea stabilitatii pozitiei GPS cand dispozitivul este nemiscat");
    Serial.println("- verificarea validarii pozitiei prin fuziunea GPS-IMU");
    Serial.println("- detectarea eventualelor salturi GPS suspecte");
    Serial.println();
    Serial.println("Conditii test:");
    Serial.println("- dispozitiv plasat in exterior");
    Serial.println("- dispozitiv mentinut nemiscat aproximativ 5 minute");
    Serial.println("- cer cat mai liber");
    Serial.println();
    Serial.println("Coduri LocationSource:");
    Serial.println("0 = pozitie indisponibila");
    Serial.println("1 = GPS curent acceptat");
    Serial.println("2 = ultima pozitie valida");
    Serial.println("3 = GPS respins ca outlier");
    Serial.println();

    gps_init();

    if (!IMU_init())
    {
        Serial.println("EROARE: IMU nu a fost initializat.");
    }
    else
    {
        Serial.println("IMU initializat corect.");
    }

    startTime = millis();

    Serial.println();
    Serial.println("Test pornit. Se asteapta date GPS si rezultat de fuziune...");
    Serial.println();
}

void loop()
{
    static unsigned long lastImuTime = 0;
    static unsigned long lastPrintTime = 0;
    static IMUdata imuData;

    static unsigned int sampleCounter = 0;
    static unsigned int gpsCurrentCount = 0;
    static unsigned int lastKnownCount = 0;
    static unsigned int rejectedCount = 0;
    static unsigned int unavailableCount = 0;

    unsigned long now = millis();

    GpsData gpsData = gps_read();

    if (now - lastImuTime >= IMU_INTERVAL_MS)
    {
        lastImuTime = now;

        if (IMU_read(imuData))
        {
            imuData = IMU_interpret(imuData);
        }
    }

    FusionData fusionData = fusion_update(gpsData, imuData);

    if (now - lastPrintTime >= PRINT_INTERVAL_MS)
    {
        lastPrintTime = now;
        sampleCounter++;

        switch (fusionData.locationSource)
        {
            case LOCATION_GPS_CURRENT:
                gpsCurrentCount++;
                break;

            case LOCATION_LAST_KNOWN:
                lastKnownCount++;
                break;

            case LOCATION_REJECTED_OUTLIER:
                rejectedCount++;
                break;

            case LOCATION_UNAVAILABLE:
            default:
                unavailableCount++;
                break;
        }

        Serial.println("--------------------------------------------------");
        Serial.print("Esantion: ");
        Serial.println(sampleCounter);

        Serial.print("Timp rulare: ");
        Serial.print((now - startTime) / 1000.0f, 1);
        Serial.println(" s");

        Serial.println();

        Serial.println("Date GPS brute:");
        Serial.print("Actualizare GPS valida acum: ");
        Serial.println(gpsData.valid ? "DA" : "NU");

        Serial.print("Ora GPS: ");
        Serial.println(gpsData.timestamp);

        Serial.print("Numar sateliti: ");
        Serial.println(gpsData.satCount);

        Serial.print("Latitudine GPS: ");
        Serial.println(gpsData.latitude, 6);

        Serial.print("Longitudine GPS: ");
        Serial.println(gpsData.longitude, 6);

        Serial.print("Altitudine GPS: ");
        Serial.print(gpsData.altitude, 2);
        Serial.println(" m");

        Serial.println();

        Serial.println("Date IMU:");
        Serial.print("Stare miscare: ");
        Serial.print(motionStateToText(imuData.motionState));
        Serial.print(" (cod ");
        Serial.print((int)imuData.motionState);
        Serial.println(")");

        Serial.print("ASVM: ");
        Serial.print(imuData.asvm, 3);
        Serial.println(" g");

        Serial.print("GSVM: ");
        Serial.print(imuData.gsvm, 2);
        Serial.println(" dps");

        Serial.println();

        Serial.println("Rezultat fuziune GPS-IMU:");
        Serial.print("Pozitie finala valida: ");
        Serial.println(fusionData.locationValid ? "DA" : "NU");

        Serial.print("Sursa pozitiei: ");
        Serial.print(locationSourceToText(fusionData.locationSource));
        Serial.print(" (cod ");
        Serial.print((int)fusionData.locationSource);
        Serial.println(")");

        Serial.print("Nivel incredere: ");
        Serial.print(fusionData.locationConfidence);
        Serial.println(" / 100");

        Serial.print("Latitudine finala: ");
        Serial.println(fusionData.latitude, 6);

        Serial.print("Longitudine finala: ");
        Serial.println(fusionData.longitude, 6);

        Serial.print("Altitudine finala: ");
        Serial.print(fusionData.altitude, 2);
        Serial.println(" m");

        Serial.println();

        Serial.println("Rezumat test pana acum:");
        Serial.print("GPS curent acceptat: ");
        Serial.println(gpsCurrentCount);

        Serial.print("Ultima pozitie valida folosita: ");
        Serial.println(lastKnownCount);

        Serial.print("Salturi GPS respinse: ");
        Serial.println(rejectedCount);

        Serial.print("Pozitie indisponibila: ");
        Serial.println(unavailableCount);

        Serial.print("Status test: ");

        if (fusionData.locationSource == LOCATION_GPS_CURRENT &&
            imuData.motionState == STATIONARY)
        {
            Serial.println("OK - GPS acceptat, dispozitiv stationar");
        }
        else if (fusionData.locationSource == LOCATION_REJECTED_OUTLIER)
        {
            Serial.println("OK - salt GPS suspect respins de fuziune");
        }
        else if (fusionData.locationSource == LOCATION_LAST_KNOWN)
        {
            Serial.println("OK - se foloseste ultima pozitie valida");
        }
        else if (fusionData.locationSource == LOCATION_UNAVAILABLE)
        {
            Serial.println("ASTEPTARE - nu exista inca pozitie valida");
        }
        else
        {
            Serial.println("IN ANALIZA");
        }

        Serial.println("--------------------------------------------------");
        Serial.println();
    }
}