#include "lora_comm.h"
#include "Arduino.h"
#include <SPI.h>
#include <LoRa.h>
#include "config.h"

static bool loraReady = false;

bool lora_init()
{
    LoRa.setPins(LORA_CS, LORA_RST, LORA_INT);

    if (!LoRa.begin(LORA_FRQ))
    {
        Serial.println("LoRa FAIL");
        loraReady = false;
        return false;
    
    }
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setTxPower(20); // Setează puterea de transmisie (0-20 dBm)
    
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
        return true; // deja inițializat
    }

    Serial.println("Retrying LoRa initialization...");
    return lora_init();
}

bool lora_send(const char* message)
{   
    if (!loraReady) {
        Serial.println("LoRa unavailable. Packet not sent.");
        return false;
    }


    LoRa.beginPacket();
    LoRa.print(message); // Adaugă un mesaj la pachet

    bool sentSuccessfully = LoRa.endPacket() == 1;

    if (!sentSuccessfully) {
        Serial.println("Eroare transmitere LoRa");
        loraReady = false; // Marchez LoRa ca nefuncțional pentru a încerca reinițializarea la următoarea trimitere
        return false;
    }

   Serial.print("Sent: ");
   Serial.println(message);
   return true;
}