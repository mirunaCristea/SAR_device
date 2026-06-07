#include <Arduino.h>
#include "imu.h"
#include <Arduino_LSM9DS1.h>

//#include <Arduino_BMI270_BMM150.h>



/*
State 1 → SVM < 0.8g           → trigger
State 2 → SVM > 1.4g în 200ms  → impact
State 3 → σAcc < 100mg          → semnal stabilizat
State 4 → σGyro < 10dps         → semnal stabilizat  
State 5 → unghi ψ < 60°         → postura culcat → FALL
DIN LUCRAREA LUI TSENG !
*/


void IMU_init() {
    if(!IMU.begin())
    {
        Serial.println("IMU Error");
        while(true);
    }
    else
    {
        Serial.println("IMU OK");
    }

}

IMUdata IMU_read()

{   IMUdata data;
    if(IMU.accelerationAvailable())
    {   
        IMU.readAcceleration(data.ax,data.ay,data.az);
        data.asvm = sqrt(data.ax*data.ax + data.ay*data.ay + data.az*data.az);
    }
    if(IMU.gyroscopeAvailable())
    {   
        IMU.readGyroscope(data.gx,data.gy,data.gz);
        data.gsvm = sqrt(data.gx*data.gx + data.gy*data.gy + data.gz*data.gz);
    }
    
    return data;
}

IMUdata IMU_interpret(IMUdata data)
{   
    data.motionState = MOTION_UNKNOWN;
    data.imuState = IMU_NORMAL;
    data.fallFlag = false;  
    static int state = 1 ;
    static int sampleCount = 0;
    static float asvmBuffer[200];  
    static float gsvmBuffer[200];
    static float axBuffer[200];
    static float ayBuffer[200];
    static float azBuffer[200];
    float unghi_inclinare = 0;
    static bool state2Criteria=0;

    // valorile statice se acumuleaza intre apeluri 

    if (data.asvm > 0.9 && data.asvm < 1.1 && data.gsvm < 10) {
    data.motionState = STATIONARY;
    } else {
        data.motionState = MOVING;
    }


    
    if (state == 1)
    {
        if (data.asvm < 0.8)
        {  
            state =2;
            data.imuState = IMU_POSSIBLE_FALL;
            Serial.println();
            Serial.println("===== ANALIZA CADERE IMU =====");
            Serial.print("[1] Acceleratie redusa detectata: ");
            Serial.print(data.asvm, 3);
            Serial.println(" g");
            Serial.println("[2] Se cauta impactul si se colecteaza 200 esantioane...");
            return data;
        }
    }


    if(state == 2)
    {
        asvmBuffer[sampleCount]=data.asvm;
        if (state2Criteria==0 && data.asvm >1.4)
        {   state2Criteria=1;
            data.imuState = IMU_IMPACT;
            Serial.print("[3] Impact detectat la esantionul ");
            Serial.print(sampleCount + 1);
            Serial.print(", aSVM = ");
            Serial.print(data.asvm, 3);
            Serial.println(" g");
        }

        gsvmBuffer[sampleCount]=data.gsvm;
        axBuffer[sampleCount]=data.ax;
        ayBuffer[sampleCount]=data.ay;
        azBuffer[sampleCount]=data.az;
        sampleCount++;

        if (sampleCount % 25 == 0 && sampleCount < 200)
        {
            Serial.print("[4] Colectare post-impact: ");
            Serial.print(sampleCount);
            Serial.println("/200 esantioane");
        }

        if (sampleCount == 200)
        {   
            float meanAcc =0;
            float meanGyro =0;
            float deviationAcc =0;
            float deviationGyro =0;
            for(int i=150; i<200; ++i)
            {
                meanAcc+=asvmBuffer[i];
                meanGyro+=gsvmBuffer[i];
            }                    
            meanAcc = meanAcc*0.02;
            meanGyro = meanGyro*0.02;
            
            for(int i=150; i<200; ++i)
            {
                deviationAcc+=(asvmBuffer[i] - meanAcc)*(asvmBuffer[i] - meanAcc);
                deviationGyro+=(gsvmBuffer[i] - meanGyro)*(gsvmBuffer[i] -meanGyro);

            }
            deviationAcc=sqrt(deviationAcc*0.02);
            deviationGyro=sqrt(deviationGyro*0.02);
            Serial.println("[5] Fereastra de analiza este completa.");
            Serial.print("    Deviatie acceleratie: ");
            Serial.print(deviationAcc, 3);
            Serial.println(" g");
            Serial.print("    Deviatie giroscop:    ");
            Serial.print(deviationGyro, 2);
            Serial.println(" dps");

            if (state2Criteria && deviationAcc <0.15 )
            {
                state =3;
                Serial.println("[6] Acceleratia s-a stabilizat.");

                if(deviationGyro <10)
                {
                    state =4;
                    Serial.println("[7] Miscarea de rotatie s-a stabilizat.");

                    for (int i = 180; i<200; ++i)
                    {   
                        unghi_inclinare+=atan2(ayBuffer[i],sqrt(axBuffer[i]*axBuffer[i] + azBuffer[i]*azBuffer[i])) * 180 / PI;
                    }
                    unghi_inclinare=unghi_inclinare*0.05;
                    Serial.print("[8] Unghi mediu de inclinare: ");
                    Serial.print(unghi_inclinare, 2);
                    Serial.println(" grade");
                    if (unghi_inclinare < 60)
                    {   state = 5;
                        data.imuState = IMU_CONFIRMED_FALL;
                        data.motionState = IMMOBILE;
                        data.fallFlag = true;
                        Serial.println("[9] DECIZIE: CADERE CONFIRMATA");
                    }
                    else
                    {
                        Serial.println("[9] DECIZIE: postura nu confirma caderea");
                    }

                }
                else
                {
                    Serial.println("[7] Miscare detectata: caderea nu este confirmata");
                }
            }
            else
            {
                Serial.println("[6] Semnal nestabil sau impact absent");
                Serial.println("[9] DECIZIE: caderea nu este confirmata");
            }

            Serial.println("==============================");


            state = 1;
            sampleCount = 0;
            state2Criteria = 0;
            meanAcc = 0; meanGyro = 0;
            deviationAcc = 0; deviationGyro = 0;
            unghi_inclinare = 0;
           

        }

    }    

    return data;
    
}

// bool IMU_getFallFlag() 
// {
//     bool flag = fallFlag;
//     fallFlag = false; // Resetăm flag-ul după ce a fost citit
//     Serial.println("Fall flag read: " + String(flag));
//     return flag;
// }
