#include <Arduino.h>
#include "gps/gps.h"

static const unsigned long PRINT_INTERVAL_MS = 1000;

static bool firstFixDetected = false;
static unsigned long startTime = 0;
static unsigned long firstFixTime = 0;

void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println(" TEST 2 - VALIDARE GPS IN EXTERIOR");
    Serial.println(" Obiectiv: masurarea timpului pana la primul fix GPS");
    Serial.println("==============================================");
    Serial.println();
    Serial.println("Conditii test:");
    Serial.println("- dispozitiv plasat in exterior");
    Serial.println("- cer cat mai liber");
    Serial.println("- se urmareste aparitia primului fix GPS valid");
    Serial.println();
    Serial.println("Asteptat:");
    Serial.println("- Fix GPS obtinut: DA");
    Serial.println("- sateliti > 0");
    Serial.println("- timp GPS actualizat");
    Serial.println("- coordonate diferite de 0");
    Serial.println();

    gps_init();

    startTime = millis();
}

void loop()
{
    static unsigned long lastPrintTime = 0;
    static unsigned int sampleCounter = 0;

    GpsData gpsData = gps_read();
    unsigned long now = millis();

    if (gpsData.valid && !firstFixDetected)
    {
        firstFixDetected = true;
        firstFixTime = now - startTime;

        Serial.println();
        Serial.println("**********************************************");
        Serial.println(">>> PRIMUL FIX GPS A FOST OBTINUT <<<");
        Serial.print("Timp pana la primul fix: ");
        Serial.print(firstFixTime / 1000.0f, 2);
        Serial.println(" s");
        Serial.println("**********************************************");
        Serial.println();
    }

    if (now - lastPrintTime >= PRINT_INTERVAL_MS)
    {
        lastPrintTime = now;
        sampleCounter++;

        Serial.println("----------------------------------------------");
        Serial.print("Esantion: ");
        Serial.println(sampleCounter);

        Serial.print("Timp rulare: ");
        Serial.print((now - startTime) / 1000.0f, 1);
        Serial.println(" s");

        Serial.print("Fix GPS obtinut: ");
        Serial.println(firstFixDetected ? "DA" : "NU");

        Serial.print("Actualizare pozitie acum: ");
        Serial.println(gpsData.valid ? "DA" : "NU");

        Serial.print("Timp pana la primul fix: ");
        if (firstFixDetected)
        {
            Serial.print(firstFixTime / 1000.0f, 2);
            Serial.println(" s");
        }
        else
        {
            Serial.println("in asteptare");
        }

        Serial.print("Ora GPS: ");
        Serial.println(gpsData.timestamp);

        Serial.print("Numar sateliti: ");
        Serial.println(gpsData.satCount);

        Serial.print("Latitudine: ");
        Serial.println(gpsData.latitude, 6);

        Serial.print("Longitudine: ");
        Serial.println(gpsData.longitude, 6);

        Serial.print("Altitudine: ");
        Serial.print(gpsData.altitude, 2);
        Serial.println(" m");

        Serial.print("Status test: ");
        if (firstFixDetected)
        {
            Serial.println("GPS VALIDAT - date disponibile pentru mesajul LoRa");
        }
        else
        {
            Serial.println("ASTEAPTARE FIX GPS");
        }

        Serial.println("----------------------------------------------");
        Serial.println();
    }
}