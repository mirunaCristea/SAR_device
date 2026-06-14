#ifndef LORA_COMM_H
#define LORA_COMM_H
bool lora_init();
bool lora_send(const char* message); //nu aloca memorie dinamică, folosește un buffer prealocat; mai eficient dpdv al memoriei și performanței

bool lora_isReady();
bool lora_retryInit();


#endif // LORA_COMM_H