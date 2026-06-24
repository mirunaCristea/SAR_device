#include <Arduino.h>
#include "imu.h"
#include <Arduino_LSM9DS1.h>
/*
Algoritm de detecție a căderii pe baza unei mașini cu stări finite:

State 1: monitorizare normală
State 2: posibilă cădere, declanșată de ASVM < 0.8 g
State 3: impact detectat dacă ASVM > 1.4 g
State 4: verificare stabilizare pe baza variației accelerației și giroscopului
State 5: confirmare cădere pe baza posturii finale

Logica urmărește succesiunea: accelerație redusă -> impact -> stabilizare -> postură finală.
*/

bool IMU_init()
{
    if (!IMU.begin()) {
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

    // Modulul vectorului accelerație, folosit pentru detecția șocurilor.
    data.asvm = sqrt(
        data.ax * data.ax +
        data.ay * data.ay +
        data.az * data.az
    );

    // Modulul vectorului vitezei unghiulare, folosit pentru estimarea mișcării.
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

    // Variabile statice folosite pentru păstrarea stării algoritmului
    // între apeluri succesive ale funcției.
    static int state = 1;
    static int sampleCount = 0;

    static float asvmBuffer[200];
    static float gsvmBuffer[200];
    static float axBuffer[200];
    static float ayBuffer[200];
    static float azBuffer[200];

    static bool impactDetected = false;
    static unsigned long collectionStartTime = 0;

    float unghi_inclinare = 0;

    // Estimare simplă a stării de mișcare pe baza accelerației și giroscopului.
    if (data.asvm > 0.9 && data.asvm < 1.1 && data.gsvm < 10) {
        data.motionState = STATIONARY;
    }
    else {
        data.motionState = MOVING;
    }

    // State 1: monitorizare normală.
    // O valoare redusă a accelerației poate indica începutul unei căderi.
    if (state == 1) {
        if (data.asvm < 0.8) {
            state = 2;
            sampleCount = 0;
            impactDetected = false;
            collectionStartTime = millis();

            data.imuState = IMU_POSSIBLE_FALL;
            return data;
        }
    }

    // State 2: colectarea unei ferestre de eșantioane după trigger.
    if (state == 2) {
        data.imuState = IMU_POSSIBLE_FALL;

        if (sampleCount < 200) {
            gsvmBuffer[sampleCount] = data.gsvm;
            asvmBuffer[sampleCount] = data.asvm;
            axBuffer[sampleCount] = data.ax;
            ayBuffer[sampleCount] = data.ay;
            azBuffer[sampleCount] = data.az;

            if (!impactDetected && data.asvm > 1.4) {
                impactDetected = true;
                data.imuState = IMU_IMPACT;

                Serial.println("STAREA 2: IMPACT!");
            }

            sampleCount++;
        }

        // După completarea ferestrei, se verifică dacă impactul a fost urmat
        // de stabilizarea semnalelor inerțiale.
        if (sampleCount >= 200) {
            Serial.print("Durata ferestrei de 200 esantioane: ");
            Serial.print(millis() - collectionStartTime);
            Serial.println(" ms");

            if (!impactDetected) {
                Serial.println("Impact absent in fereastra. Revenire la normal.");

                state = 1;
                sampleCount = 0;
                impactDetected = false;
                data.imuState = IMU_NORMAL;

                return data;
            }

            float meanAcc = 0;
            float meanGyro = 0;
            float deviationAcc = 0;
            float deviationGyro = 0;

            // Se analizează ultima parte a ferestrei pentru a verifica
            // dacă dispozitivul s-a stabilizat după impact.
            for (int i = 150; i < 200; ++i) {
                meanAcc += asvmBuffer[i];
                meanGyro += gsvmBuffer[i];
            }

            meanAcc = meanAcc * 0.02;
            meanGyro = meanGyro * 0.02;

            for (int i = 150; i < 200; ++i) {
                deviationAcc +=
                    (asvmBuffer[i] - meanAcc) *
                    (asvmBuffer[i] - meanAcc);

                deviationGyro +=
                    (gsvmBuffer[i] - meanGyro) *
                    (gsvmBuffer[i] - meanGyro);
            }

            deviationAcc = sqrt(deviationAcc * 0.02);
            deviationGyro = sqrt(deviationGyro * 0.02);

            // State 3: accelerația este stabilizată după impact.
            if (impactDetected && deviationAcc < 0.15) {
                state = 3;
                Serial.println("STAREA 3: Semnal Stabilizat");

                // State 4: se verifică și stabilizarea vitezei unghiulare.
                if (deviationGyro < 10) {
                    state = 4;
                    Serial.println("STAREA 4: Semnal Stabilizat");

                    // Estimarea posturii finale folosind ultimele eșantioane.
                    for (int i = 180; i < 200; ++i) {
                        unghi_inclinare += atan2(
                            ayBuffer[i],
                            sqrt(
                                axBuffer[i] * axBuffer[i] +
                                azBuffer[i] * azBuffer[i]
                            )
                        ) * 180 / PI;
                    }

                    unghi_inclinare = unghi_inclinare * 0.05;

                    // State 5: confirmarea căderii pe baza unghiului final.
                    if (fabsf(unghi_inclinare) < 60.0f) {
                        state = 5;
                        data.imuState = IMU_CONFIRMED_FALL;
                        data.motionState = IMMOBILE;
                        data.fallFlag = true;

                        Serial.println("STAREA 5: CAZATURA DETECTATA!");
                    }
                }
            }

            // Resetarea mașinii cu stări finite pentru următoarea detecție.
            state = 1;
            sampleCount = 0;
            impactDetected = false;
        }
    }

    return data;
}
