#include "packet.h"
#include "imu/imu.h"
/* STRUCTURA PACHET:
"%s;%04ld;%.6f;%.6f;%.2f;%s;%d;%d;%d"
callSign; counter; latitude; longitude; altitude; timestamp; satCount; fallFlag; battery
ex: NODE1;0042;44.367260;26.145315;113.10;18:24:26;7;1;0;A3F2
1.  NODE1 — call sign
2.  0042 — packet counter
3.  44.367260 — latitudine
4.  26.145315 — longitudine
5.  113.10 — altitudine
6.  18:24:26 — timestamp
7.  7 — sateliți
8. battery — nivelul bateriei
9. eventType — tipul evenimentului (normal, căzătură, ajutor audio, baterie descărcată)
10. alertLevel — nivelul alertei (0-4)
11. locationSource — sursa poziției (GPS curent, ultima cunoscută, indisponibilă)
12. locationConfidence — încrederea în poziție (0-100)
13. CRC16 — cod de verificare pentru integritatea datelor (4 hexazecimale)


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
String packet_build(PacketData data)
{
    char message[200];
    snprintf(message, sizeof(message), "%s;%04ld;%.6f;%.6f;%.2f;%s;%d;%d;%d;%d;%d;%d", 
    data.callSign,          //%s - char array
    data.counter,           //%04ld - long int, with at least 4 digits
    data.gpsData.latitude,  //%.6f - float with 6 decimal places
    data.gpsData.longitude, //%.6f - float with 6 decimal places
    data.gpsData.altitude,  //%.2f - float with 2 decimal places
    data.gpsData.timestamp, //%s - char array
    data.gpsData.satCount,  //%d - int
    data.battery,            //%d - int
    data.alertData.eventType, //%d - int
    data.alertData.alertLevel, //%d - int
    (uint8_t) data.locationSource,     //%d - int
    data.locationConfidence  //%d - int
    );
    
    uint16_t crc=crc16(message);
    char crcStr[10];
    snprintf(crcStr, sizeof(crcStr), ";%04X", crc);
    strcat(message, crcStr);
    return String(message);
}