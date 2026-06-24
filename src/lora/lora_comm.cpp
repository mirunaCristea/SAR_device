#include "lora_comm.h"
#include "Arduino.h"
#include <SPI.h>
#include <LoRa.h>
#include "config.h"

// Reține starea curentă a modulului LoRa pentru a permite funcționarea sistemului și în mod degradat, fără blocarea aplicației.
static bool loraReady = false;

bool lora_init()
{
    LoRa.setPins(LORA_CS, LORA_RST, LORA_INT);

    if (!LoRa.begin(LORA_FRQ)) {
        Serial.println("LoRa FAIL");
        loraReady = false;
        return false;
    }

    // Configurarea parametrilor radio utilizați pentru transmisia pachetelor.
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setTxPower(20);

    loraReady = true;
    Serial.println("LoRa OK");

    return true;
}

bool lora_isReady()
{
    return loraReady;
}

bool lora_retryInit()
{
    if (loraReady) {
        return true;
    }

    // Reîncercare de inițializare după o pornire eșuată sau o eroare de transmisie.
    Serial.println("Retrying LoRa initialization...");
    return lora_init();
}

bool lora_send(const char* message)
{
    if (!loraReady) {
        Serial.println("LoRa unavailable. Packet not sent.");
        return false;
    }

    // Mesajul este primit deja formatat și este transmis ca payload LoRa.
    LoRa.beginPacket();
    LoRa.print(message);

    bool sentSuccessfully = LoRa.endPacket() == 1;

    if (!sentSuccessfully) {
        Serial.println("Eroare transmitere LoRa");

        // Modulul este marcat indisponibil pentru a permite reinițializarea ulterioară.
        loraReady = false;
        return false;
    }

    Serial.print("Sent: ");
    Serial.println(message);

    return true;
}

