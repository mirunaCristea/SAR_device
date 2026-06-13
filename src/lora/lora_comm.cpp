#include "lora_comm.h"
#include "Arduino.h"
#include <SPI.h>
#include <LoRa.h>
#include "config.h"



void lora_init()
{
    LoRa.setPins(LORA_CS, LORA_RST, LORA_INT);
    if (!LoRa.begin(LORA_FRQ))
    {
        Serial.println("LoRa FAIL");
        while (true);
    
    }
    Serial.println("LoRa OK");
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setTxPower(20); // Setează puterea de transmisie (0-20 dBm)

}

bool lora_send(const char* message)
{
    LoRa.beginPacket();
    LoRa.print(message); // Adaugă un mesaj la pachet

    bool sentSuccessfully = LoRa.endPacket() == 1;

    if (!sentSuccessfully) {
        Serial.println("Eroare transmitere LoRa");
        return false;
    }

   Serial.print("Sent: ");
   Serial.println(message);
   return true;
}