#ifndef LORA_COMM_H
#define LORA_COMM_H

// Inițializează modulul LoRa și configurează parametrii radio principali.
bool lora_init();

// Transmite un mesaj deja construit prin interfața LoRa.
bool lora_send(const char* message);

// Indică dacă modulul LoRa este disponibil pentru transmitere.
bool lora_isReady();

// Reîncearcă inițializarea modulului LoRa în cazul funcționării degradate.
bool lora_retryInit();

#endif // LORA_COMM_H

