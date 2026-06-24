#include <Arduino.h>

#include "alert/alert.h"
#include "audio/audio.h"
#include "config.h"
#include "fusion/fusion.h"
#include "gps/gps.h"
#include "imu/imu.h"
#include "lora/lora_comm.h"
#include "lora/packet.h"
#include "power/power.h"
#include "route/route.h"

// ===================== TIMING =====================

// Intervale de execuție pentru subsistemele care nu trebuie rulate continuu.
// Se folosește millis(), nu delay(), pentru a păstra bucla principală non-blocantă.
static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long SEND_INTERVAL_MS = 5000;
static const unsigned long SEND_RETRY_INTERVAL_MS = 5000;
static const unsigned long LORA_RETRY_INTERVAL_MS = 15000;
static const unsigned long BATTERY_READ_INTERVAL_MS = 5000;
static const unsigned long DEBUG_INTERVAL_MS = 1000;

// ===================== STATE =====================

// Contor incrementat la fiecare pachet transmis, folosit pentru identificarea
// ordinii mesajelor la recepție și pentru observarea eventualelor pierderi.
static long packetCounter = 0;

// Marcaje temporale pentru execuția periodică a modulelor.
static unsigned long lastImuTime = 0;
static unsigned long lastSendTime = 0;
static unsigned long lastSendAttemptTime = 0;
static unsigned long lastLoRaRetryTime = 0;
static unsigned long lastBatteryReadTime = 0;
static unsigned long lastDebugTime = 0;

// Structuri globale care păstrează cea mai recentă stare validă a subsistemelor.
static IMUdata imuData;
static AudioData audioData;
static BatteryData batteryData;
static RouteData routeData;

// ===================== INIT =====================

void setup()
{
    Serial.begin(115200);
    delay(5000);

    // Modulul LoRa este tratat separat deoarece sistemul poate funcționa local
    // și în mod degradat, chiar dacă transmisia radio nu este disponibilă inițial.
    bool loraOk = lora_init();

    if (!loraOk) {
        Serial.println("Sistem pornit in mod degradat: LoRa indisponibil");
    }

    gps_init();
    IMU_init();
    audio_init();
    battery_init();

    // Buzzer-ul este folosit pentru avertizări locale, independent de transmisia LoRa.
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Prima citire a bateriei initializează valoarea folosită în pachetele transmise.
    batteryData = battery_read();

    Serial.println("Sistem initializat");
}

// ===================== UPDATE FUNCTIONS =====================

static void update_imu(unsigned long now)
{
    if (now - lastImuTime < IMU_INTERVAL_MS) {
        return;
    }

    lastImuTime = now;

    // Datele IMU sunt citite periodic și interpretate local pentru determinarea
    // stării de mișcare și a posibilelor evenimente de tip cădere.
    if (IMU_read(imuData)) {
        imuData = IMU_interpret(imuData);
    }
}

static bool is_imu_critical_window()
{
    return imuData.imuState == IMU_POSSIBLE_FALL ||
           imuData.imuState == IMU_IMPACT;
}

static void update_audio()
{
    // Inferența audio este suspendată temporar în fereastra critică IMU pentru
    // a prioritiza detecția căderii și pentru a evita încărcarea suplimentară.
    if (is_imu_critical_window()) {
        return;
    }

    // Modulul audio actualizează rezultatul doar când o fereastră audio completă
    // a fost captată și procesată.
    if (audio_update(audioData)) {
        Serial.print("Audio: ");
        Serial.print(audioData.state == AUDIO_HELP_DETECTED ? "HELP" : "NORMAL");
        Serial.print(" | score=");
        Serial.println(audioData.helpScore);
    }
}

static void update_battery(unsigned long now)
{
    if (now - lastBatteryReadTime < BATTERY_READ_INTERVAL_MS) {
        return;
    }

    lastBatteryReadTime = now;

    // Citirea bateriei este limitată temporal deoarece variațiile sunt lente
    // și nu justifică măsurarea la fiecare iterație a buclei principale.
    batteryData = battery_read();
}

static void retry_loRa_if_needed(unsigned long now)
{
    if (lora_isReady()) {
        return;
    }

    if (now - lastLoRaRetryTime < LORA_RETRY_INTERVAL_MS) {
        return;
    }

    lastLoRaRetryTime = now;

    // Reinițializare periodică a modulului radio în cazul în care sistemul
    // a pornit în mod degradat sau comunicația LoRa a devenit indisponibilă.
    lora_retryInit();
}

static void print_debug(
    unsigned long now,
    const FusionData& fusionData
)
{
    if (now - lastDebugTime < DEBUG_INTERVAL_MS) {
        return;
    }

    lastDebugTime = now;

    // Mesaj de diagnostic folosit în testare pentru verificarea fuziunii,
    // a stării traseului și a nivelului bateriei.
    Serial.print("Fusion source=");
    Serial.print((int)fusionData.locationSource);

    Serial.print(" conf=");
    Serial.print(fusionData.locationConfidence);

    Serial.print(" valid=");
    Serial.print(fusionData.locationValid);

    Serial.print(" lat=");
    Serial.print(fusionData.latitude, 6);

    Serial.print(" lon=");
    Serial.print(fusionData.longitude, 6);

    Serial.print(" | Route=");
    Serial.print(route_statusToString(routeData.status));

    Serial.print(" dist=");
    Serial.print(routeData.distanceToRouteM);

    Serial.print(" m | Battery=");
    Serial.print(batteryData.percent);

    Serial.println("%");
}

static GpsData build_fused_GpsData(
    const GpsData& gpsData,
    const FusionData& fusionData
)
{
    // Se păstrează structura GpsData pentru compatibilitate cu modulul de alertare,
    // dar poziția este înlocuită cu rezultatul validat de fuziunea GPS-IMU.
    GpsData fusedGpsData = gpsData;

    fusedGpsData.latitude = fusionData.latitude;
    fusedGpsData.longitude = fusionData.longitude;
    fusedGpsData.altitude = fusionData.altitude;
    fusedGpsData.valid = fusionData.locationValid;

    return fusedGpsData;
}

static bool should_try_send(
    unsigned long now,
    const AlertData& alertData
)
{
    bool periodicSend = now - lastSendTime >= SEND_INTERVAL_MS;
    bool emergencySend = alertData.shouldTransmitNow;

    // Evenimentele critice au prioritate și pot declanșa transmisie imediată,
    // fără a aștepta intervalul periodic de telemetrie.
    if (emergencySend) {
        return true;
    }

    if (!periodicSend) {
        return false;
    }

    // Limitare a încercărilor de trimitere, pentru a evita repetarea rapidă
    // a transmisiilor în cazul unor erori temporare.
    if (now - lastSendAttemptTime < SEND_RETRY_INTERVAL_MS) {
        return false;
    }

    return true;
}

static bool send_packet(
    const GpsData& gpsData,
    const FusionData& fusionData,
    const AlertData& alertData
)
{
    if (!lora_isReady()) {
        Serial.println("LoRa indisponibil. Alerta ramane activa local.");
        return false;
    }

    // Structura PacketData reunește informațiile esențiale care vor fi codificate
    // în payload-ul LoRa: identificator, contor, baterie, poziție și stare de alertă.
    PacketData packetData;

    strcpy(packetData.callSign, CALL_SIGN);
    packetData.counter = ++packetCounter;
    packetData.batteryPercent = batteryData.percent;
    packetData.gpsData = gpsData;
    packetData.fusionData = fusionData;
    packetData.alertData = alertData;

    String message = packet_build(packetData);

    return lora_send(message.c_str());
}

// ===================== MAIN LOOP =====================

void loop()
{
    unsigned long now = millis();

    // 1. Achiziția datelor de la senzori.
    // GPS-ul este citit la fiecare iterație pentru a nu pierde caractere NMEA
    // din bufferul serial, în timp ce IMU și bateria sunt actualizate temporizat.
    GpsData gpsData = gps_read();

    update_imu(now);
    update_audio();
    update_battery(now);

    // 2. Fuziune GPS-IMU.
    // Poziția GPS este validată contextual folosind starea de mișcare estimată
    // din datele inerțiale.
    FusionData fusionData = fusion_update(gpsData, imuData);

    // 3. Evaluarea nivelului de alertă.
    // Modulul de alertare folosește poziția fuzionată, starea IMU,
    // detecția audio și nivelul bateriei.
    GpsData fusedGpsData = build_fused_GpsData(gpsData, fusionData);

    AlertData alertData = alert_evaluate(
        fusedGpsData,
        imuData,
        audioData,
        batteryData.percent
    );

    // 4. Verificarea deviației față de traseu.
    // Avertizarea de traseu este dezactivată când există o alertă critică,
    // pentru ca buzzer-ul local să nu suprapună semnalizări cu priorități diferite.
    bool criticalAlertActive =
        alertData.alertLevel >= 3 ||
        alertData.shouldTransmitNow;

    bool allowRouteBuzzer = !criticalAlertActive;

    routeData = route_update(fusionData, allowRouteBuzzer);

    // 5. Diagnostic periodic pentru testare și validare.
    print_debug(now, fusionData);

    // 6. Reinițializare LoRa în caz de funcționare degradată.
    retry_loRa_if_needed(now);

    // 7. Decizia de transmisie.
    // Sistemul transmite periodic statusul sau imediat în cazul unei alerte.
    if (!should_try_send(now, alertData)) {
        return;
    }

    lastSendAttemptTime = now;

    // 8. Construirea și transmiterea pachetului LoRa.
    bool sentSuccessfully = send_packet(
        gpsData,
        fusionData,
        alertData
    );

    if (sentSuccessfully) {
        lastSendTime = now;

        // După transmiterea cu succes, evenimentul critic este marcat ca trimis
        // pentru a evita retransmiterea continuă a aceleiași alerte.
        if (alertData.shouldTransmitNow) {
            alert_markSent(alertData.eventType);
        }
    }
}
