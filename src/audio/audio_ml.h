#ifndef AUDIO_ML_H
#define AUDIO_ML_H

#include <stdint.h>

bool audio_ml_init();
bool audio_ml_startCapture();
bool audio_ml_captureReady();
bool audio_ml_process();

float audio_ml_getHelpScore();

bool audio_ml_isHelpCandidate();
bool audio_ml_isHelpStrong();

#endif