#ifndef AUDIO_ML_H
#define AUDIO_ML_H

#include <stdint.h>

bool audio_ml_init();
bool audio_ml_update();
float audio_ml_getHelpScore();
float audio_ml_getBestOtherScore();
float audio_ml_getMargin();
float audio_ml_getRms();
float audio_ml_getMean();
int16_t audio_ml_getMinSample();
int16_t audio_ml_getMaxSample();
float audio_ml_getClippingPercent();
bool audio_ml_isHelpDetected();

#endif
