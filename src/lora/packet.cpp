#include "packet.h"
#include <string.h>
#include <Arduino.h>
#include <math.h>

/*
Payload LoRa compact:

ID;CNT;LAT_E6;LON_E6;ALT_M;TIME;BAT;STATUS;CRC

Exemplu:
N1;0042;44367260;26145315;113;182426;84;133;A3F2

Coordonatele sunt scalate cu 1 000 000 pentru a evita transmiterea
valorilor float și pentru a reduce dimensiunea mesajului. Timpul GPS
este transmis în format compact HHMMSS, iar CRC-ul este adăugat la final
pentru verificarea integrității payload-ului.
*/

// Calcul CRC-16 utilizat pentru verificarea payload-ului transmis.
uint16_t crc16(String data)
{
    uint16_t crc = 0xFFFF;

    for (unsigned int i = 0; i < data.length(); i++) {
        crc ^= (uint8_t)data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            }
            else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

// Aplică offset-ul de fus orar peste timpul GPS, care este primit în UTC.
char* apply_timezone_offset(
    const char* inputTime,
    char* outputTime,
    int8_t timezoneOffsetHours
)
{
    if (strlen(inputTime) < 8) {
        strcpy(outputTime, "00:00:00");
        return outputTime;
    }

    int hour = (inputTime[0] - '0') * 10 + (inputTime[1] - '0');

    hour = (hour + timezoneOffsetHours) % 24;

    if (hour < 0) {
        hour += 24;
    }

    snprintf(
        outputTime,
        9,
        "%02d:%c%c:%c%c",
        hour,
        inputTime[3],
        inputTime[4],
        inputTime[6],
        inputTime[7]
    );

    return outputTime;
}

// Transformă timpul din formatul "HH:MM:SS" în "HHMMSS",
// pentru reducerea dimensiunii mesajului transmis.
void format_time_hhmmss(const char* inputTime, char* outputTime)
{
    if (strlen(inputTime) >= 8) {
        outputTime[0] = inputTime[0];
        outputTime[1] = inputTime[1];
        outputTime[2] = inputTime[3];
        outputTime[3] = inputTime[4];
        outputTime[4] = inputTime[6];
        outputTime[5] = inputTime[7];
        outputTime[6] = '\0';
    }
    else {
        strncpy(outputTime, "000000", 7);
    }
}

uint8_t build_status(AlertData alertData, LocationSource locationSource)
{
    uint8_t alertLevel = uint8_t(alertData.alertLevel);
    uint8_t eventType = uint8_t(alertData.eventType);
    uint8_t locSource = uint8_t(locationSource);

    // Limitare la dimensiunea câmpurilor definite în STATUS.
    alertLevel = alertLevel & 0x07;
    eventType = eventType & 0x07;
    locSource = locSource & 0x03;

    return (alertLevel << 5) | (eventType << 2) | locSource;
}

String packet_build(PacketData data)
{
    char message[120];
    char timeStr[7];
    char gpsTimeStr[9];

    apply_timezone_offset(
        data.gpsData.timestamp,
        gpsTimeStr,
        TIMEZONE_OFFSET_HOURS
    );

    format_time_hhmmss(gpsTimeStr, timeStr);

    long latE6 = 0;
    long lonE6 = 0;
    int altM = 0;

    // Dacă poziția este validă, coordonatele sunt convertite în valori întregi
    // scalate, pentru un payload LoRa mai scurt și mai ușor de decodat.
    if (data.fusionData.locationValid) {
        latE6 = lroundf(data.fusionData.latitude * 1000000.0f);
        lonE6 = lroundf(data.fusionData.longitude * 1000000.0f);
        altM = round(data.fusionData.altitude);
    }

    uint8_t status = build_status(
        data.alertData,
        data.fusionData.locationSource
    );

    // Construirea mesajului fără CRC; CRC-ul se calculează peste acest conținut.
    snprintf(
        message,
        sizeof(message),
        "%s;%04ld;%ld;%ld;%d;%s;%d;%u",
        data.callSign,
        data.counter,
        latE6,
        lonE6,
        altM,
        timeStr,
        data.batteryPercent,
        (unsigned int)status
    );

    uint16_t crc = crc16(message);

    char crcStr[10];
    snprintf(crcStr, sizeof(crcStr), ";%04X", crc);

    // Adăugarea CRC-ului la final, fără depășirea bufferului alocat.
    strncat(
        message,
        crcStr,
        sizeof(message) - strlen(message) - 1
    );

    return String(message);
}
