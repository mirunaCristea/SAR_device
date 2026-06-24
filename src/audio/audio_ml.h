#ifndef AUDIO_ML_H
#define AUDIO_ML_H

#include <stdint.h>

// Inițializează microfonul PDM, bufferul audio și modelul TinyML.
bool audio_ml_init();

// Pornește captarea unei ferestre audio pentru inferență.
bool audio_ml_startCapture();

// Indică dacă fereastra audio necesară modelului este completă.
bool audio_ml_captureReady();

// Rulează inferența TinyML pe fereastra audio captată.
bool audio_ml_process();

// Returnează scorul asociat clasei de ajutor.
float audio_ml_getHelpScore();

// Indică o detecție posibilă a apelului de ajutor.
bool audio_ml_isHelpCandidate();

// Indică o detecție puternică a apelului de ajutor.
bool audio_ml_isHelpStrong();

#endif // AUDIO_ML_H