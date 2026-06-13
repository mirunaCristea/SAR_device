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


bool IMU_init() {
    if(!IMU.begin())
    {
        Serial.println("IMU Error");
        return false;
    }
    
    
    Serial.println("IMU OK");
    return true;
  

}

bool IMU_read(IMUdata &data)
{
    if (!IMU.accelerationAvailable() ||
        !IMU.gyroscopeAvailable()) {
        return false;
    }

    IMU.readAcceleration(data.ax, data.ay, data.az);
    IMU.readGyroscope(data.gx, data.gy, data.gz);

    data.asvm = sqrt(
        data.ax * data.ax +
        data.ay * data.ay +
        data.az * data.az
    );

    data.gsvm = sqrt(
        data.gx * data.gx +
        data.gy * data.gy +
        data.gz * data.gz
    );

    return true;
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
    static bool impactDetected = false;
    static unsigned long collectionStartTime = 0;
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
            sampleCount = 0; // resetează contorul de mostre pentru starea 2
            impactDetected = false; // resetează criteriul pentru starea 2
            collectionStartTime = millis(); // începe monitorizarea pentru starea 2

            data.imuState = IMU_POSSIBLE_FALL;
            return data;
        }
    }


    if(state == 2)

    {   data.imuState = IMU_POSSIBLE_FALL;
        
        
        if (sampleCount  < 200 ) {
            // Dacă a trecut timpul maxim pentru impact, resetăm starea
            gsvmBuffer[sampleCount]=data.gsvm;
            asvmBuffer[sampleCount]=data.asvm;
            axBuffer[sampleCount]=data.ax;
            ayBuffer[sampleCount]=data.ay;
            azBuffer[sampleCount]=data.az;

               


        if (!impactDetected && data.asvm >1.4)
        {  
            impactDetected=true;
            data.imuState = IMU_IMPACT;

            Serial.println("STAREA 2: IMPACT!");

            

        }

        sampleCount++;

    }


        if (sampleCount >= 200)
        {   

            Serial.print("Durata ferestrei de 200 esantioane: ");
            Serial.print(millis() - collectionStartTime);
            Serial.println(" ms");

            if (!impactDetected) {
                Serial.println("Impact absent in fereastra. Revenire la normal.");

                state = 1; // Resetăm starea dacă impactul nu a fost detectat
                sampleCount = 0;
                impactDetected = false;
                data.imuState = IMU_NORMAL;
                return data;
            }

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
            // Serial.print("Deviation Acc: ");
            // Serial.print(deviationAcc);
            // Serial.print(" g, Deviație Gyro: ");
            // Serial.println(deviationGyro);

            if (impactDetected && deviationAcc <0.15 )
            {
                state =3;
                Serial.println("STAREA 3: Semnal Stabilizat");

                if(deviationGyro <10)
                {
                    state =4;
                    Serial.println("STAREA 4: Semnal Stabilizat");

                    for (int i = 180; i<200; ++i)
                    {   
                        unghi_inclinare+=atan2(ayBuffer[i],sqrt(axBuffer[i]*axBuffer[i] + azBuffer[i]*azBuffer[i])) * 180 / PI;
                    }
                    unghi_inclinare=unghi_inclinare*0.05;
                    // Serial.print("Unghi de inclinare: ");
                    // Serial.println(unghi_inclinare);
                    if (fabsf(unghi_inclinare) < 60.0f)
                    {   state = 5;
                        data.imuState = IMU_CONFIRMED_FALL;
                        data.motionState = IMMOBILE;
                        data.fallFlag = true;
                        Serial.println("STAREA 5: CĂZĂTURĂ DETECTATĂ!");
                    }

                }
            }


            state = 1;
            sampleCount = 0;
            impactDetected = false;
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
