#include "packet.h"
#include <string.h>
#include <Arduino.h>
#include <math.h>

/*
Format compact pentru mesajul LoRa:

ID;CNT;LAT_E6;LON_E6;ALT_M;TIME;BAT;STATUS;CRC

Exemplu:
N1;0042;44367260;26145315;113;182426;84;133;A3F2

Semnificatia campurilor:
- ID       = identificatorul nodului / call sign
- CNT      = contorul pachetului transmis
- LAT_E6   = latitudine scalata cu 1 000 000
  Exemplu: 44.367260 devine 44367260.
  Aceasta reprezentare evita transmiterea valorilor float
  
  si reduce dimensiunea mesajului.
- LON_E6   = longitudine scalata cu 1 000 000
- ALT_M    = altitudine in metri (voi trimite valoare intreaga, fara zecimale, pentru a economisi spatiu    )
- TIME     = timpul GPS in format HHMMSS (fara delimitatori) pentru a economisi spatiu
  Exemplu: 18:24:26 devine 182426.
- BAT      = nivelul bateriei in procente
- STATUS   = camp compact care include nivelul alertei,
             tipul evenimentului si sursa pozitiei
- CRC      = cod CRC-16 pentru verificarea integritatii mesajului

Campul STATUS este codificat pe 8 biti astfel:

[ alertLevel ][ eventType ][ locationSource ]
     3 biti       3 biti          2 biti

Formula:
STATUS = alertLevel * 32 + eventType * 4 + locationSource

Coduri locationSource:
0 = pozitie indisponibila
1 = GPS curent valid
2 = ultima pozitie valida cunoscuta
3 = GPS curent respins ca outlier

Aceasta codare reduce dimensiunea payload-ului LoRa,
pastrand separat informatia despre severitatea alertei,
tipul evenimentului si calitatea/sursa pozitiei transmise.
*/












uint16_t crc16(String data) {
    uint16_t crc = 0xFFFF;
    for (unsigned int i = 0; i < data.length(); i++) {
        crc ^= (uint8_t)data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;

}

void apply_timezone_offset(const char* inputTime, char* outputTime, int8_t timezoneOffsetHours)
{

    if (strlen(inputTime) < 8) {
        strcpy(outputTime, "00:00:00");
        return;
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
}

void format_time_hhmmss(const char* inputTime, char* outputTime )

{   
    //inputTime: "HH:MM:SS"
    //outputTime: "HHMMSS"

    if(strlen(inputTime) >= 8)
    {
    outputTime[0] = inputTime[0];
    outputTime[1] = inputTime[1];
    outputTime[2] = inputTime[3];
    outputTime[3] = inputTime[4];
    outputTime[4] = inputTime[6];
    outputTime[5] = inputTime[7];
    outputTime[6] = '\0'; // Null-terminate the output string
    }
    else
    {
        strncpy(outputTime, "000000", 7);
    }
}

uint8_t build_status(AlertData alertData, LocationSource locationSource)
{
    uint8_t alertLevel = uint8_t(alertData.alertLevel);
    uint8_t eventType = uint8_t(alertData.eventType);
    uint8_t locSource = uint8_t(locationSource);

    // Limitare pe numarul de biti alocat fiecarui camp:
    // alertLevel     -> 3 biti -> 0..7
    // eventType      -> 3 biti -> 0..7
    // locationSource -> 2 biti -> 0..3

    alertLevel = alertLevel & 0x07; // 3 biti
    eventType = eventType & 0x07;   // 3 biti
    locSource = locSource & 0x03;   // 2 biti

    return (alertLevel << 5) | (eventType << 2) | locSource;
}




String packet_build(PacketData data)
{
    char message[120];
    char timeStr[7];
    char timeWithOffset[9];
    
    apply_timezone_offset(data.gpsData.timestamp, timeWithOffset, TIMEZONE_OFFSET_HOURS);
    format_time_hhmmss(timeWithOffset, timeStr);

    long latE6 = 0;
    long lonE6 = 0;
    int altM = 0;

    if (data.fusionData.locationValid)
    {   
        latE6 = lroundf(data.fusionData.latitude * 1000000.0f);
        lonE6 = lroundf(data.fusionData.longitude * 1000000.0f);
        altM = round(data.fusionData.altitude);
    }
 
    uint8_t status = build_status(data.alertData, data.fusionData.locationSource);

    snprintf(message, sizeof(message), 
        "%s;%04ld;%ld;%ld;%d;%s;%d;%u", 
        data.callSign, 
        data.counter, 
        latE6, 
        lonE6, 
        altM, 
        timeStr, 
        data.batteryPercent, 
       (unsigned int)status);

    
    uint16_t crc=crc16(message);
    char crcStr[10];
    snprintf(crcStr, sizeof(crcStr), ";%04X", crc);
    strncat(message, crcStr, sizeof(message) - strlen(message) - 1); // Adaugare CRC la mesaj, asigurand spatiu in buffer
    return String(message);
}