#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#include "config.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "audio/audio.h"
#include "power/power.h"

// Test scurt pentru validarea pornirii modulelor
// Scop: screenshot Serial Monitor pentru licenta

static IMUdata imuData;
static AudioData audioData;
static BatteryData batteryData;

static bool imuOk = false;
static bool loraOk = false;

bool lora_test_init()
{
    LoRa.setPins(LORA_CS, LORA_RST, LORA_INT);

    if (!LoRa.begin(LORA_FRQ)) {
        return false;
    }

    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setTxPower(20);

    return true;
}

void print_separator()
{
    Serial.println("--------------------------------------------------");
}

void setup()
{
    Serial.begin(115200);
    delay(5000);

    Serial.println();
    Serial.println("==================================================");
    Serial.println("TEST PORNIRE SI INITIALIZARE MODULE");
    Serial.println("Dispozitiv portabil SAR");
    Serial.println("==================================================");

    // 1. GPS / GNSS
    gps_init();
    Serial.println("[GPS] initializare apelata");

    // 2. IMU
    imuOk = IMU_init();

    if (imuOk) {
        Serial.println("[IMU] OK");
    } else {
        Serial.println("[IMU] EROARE");
    }

    // 3. Audio TinyML
    audio_init();
    Serial.println("[AUDIO] initializare apelata");
    Serial.println("[AUDIO] verifica mai sus mesajul: Audio ML OK");

    // 4. Baterie
    battery_init();
    batteryData = battery_read();
    Serial.println("[BATERIE] initializare apelata");

    // 5. LoRa
    loraOk = lora_test_init();

    if (loraOk) {
        Serial.println("[LoRa] OK");
    } else {
        Serial.println("[LoRa] EROARE");
    }

    print_separator();

    // Citire IMU pentru dovada ca nu doar porneste, ci si livreaza valori
    if (imuOk && IMU_read(imuData)) {
        imuData = IMU_interpret(imuData);

        Serial.print("[IMU] ax=");
        Serial.print(imuData.ax, 3);
        Serial.print(" ay=");
        Serial.print(imuData.ay, 3);
        Serial.print(" az=");
        Serial.print(imuData.az, 3);

        Serial.print(" | gx=");
        Serial.print(imuData.gx, 2);
        Serial.print(" gy=");
        Serial.print(imuData.gy, 2);
        Serial.print(" gz=");
        Serial.print(imuData.gz, 2);

        Serial.print(" | asvm=");
        Serial.print(imuData.asvm, 3);
        Serial.print(" | gsvm=");
        Serial.println(imuData.gsvm, 2);
    } else {
        Serial.println("[IMU] valori indisponibile");
    }

    // Citire baterie pentru dovada ADC
    batteryData = battery_read();

    Serial.print("[BATERIE] tensiune=");
    Serial.print(batteryData.voltage, 2);
    Serial.print(" V | procent=");
    Serial.print(batteryData.percent);
    Serial.println("%");

    // Citire rapida GPS
    GpsData gpsData = gps_read();

    Serial.print("[GPS] valid=");
    Serial.print(gpsData.valid);
    Serial.print(" | sateliti=");
    Serial.print(gpsData.satCount);
    Serial.print(" | timp=");
    Serial.print(gpsData.timestamp);
    Serial.print(" | lat=");
    Serial.print(gpsData.latitude, 6);
    Serial.print(" | lon=");
    Serial.println(gpsData.longitude, 6);

    // Test transmitere LoRa, doar daca modulul a pornit
    if (loraOk) {
        LoRa.beginPacket();
        LoRa.print("TEST_INIT_MODULES");
        int result = LoRa.endPacket();

        Serial.print("[LoRa] transmitere test=");
        Serial.println(result == 1 ? "OK" : "EROARE");
    } else {
        Serial.println("[LoRa] transmitere test anulata");
    }

    print_separator();

    Serial.println("REZUMAT TEST INITIALIZARE");
    Serial.println("GPS      : initializare apelata");
    Serial.println(imuOk  ? "IMU      : OK" : "IMU      : EROARE");
    Serial.println("Audio    : vezi mesaj Audio ML OK");
    Serial.println(loraOk ? "LoRa     : OK" : "LoRa     : EROARE");
    Serial.println("Baterie  : tensiune si procent afisate");

    print_separator();

    Serial.println("Test finalizat. Realizeaza captura pentru licenta.");
}

void loop()
{
    // Testul este one-shot.
    // Nu rulam bucla continua ca sa fie screenshot-ul curat.
}