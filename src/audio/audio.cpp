// #include <PDM.h>
// #include "audio.h"

// short sampleBuffer[256]; // Buffer pentru stocarea mostrelor audio  
// volatile int samplesRead =0;

// void onPDMdata()
// {
//     samplesRead = PDM.read(sampleBuffer, sizeof(sampleBuffer)); // Citeste mostrele audio in buffer
// }

// void mic_init()
// {
//     PDM.onReceive(onPDMdata);
//     PDM.begin(1, 16000); 

// }

// void mic_read() {
    
//     if(samplesRead > 0) {
//         float RMS = 0.0;
//         int count = samplesRead / 2;
        
//         for(int i = 0; i < count; ++i) {
//             RMS += (float)sampleBuffer[i] * sampleBuffer[i];
//         }
//         RMS = sqrt(RMS / count);
        
//         Serial.print("RMS: ");
//         Serial.println(RMS);
        
//         samplesRead = 0; // ← resetezi după ce ai procesat
//     }
// }