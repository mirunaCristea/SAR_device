#ifndef IMU_H
#define IMU_H


enum MotionState    // spune cum se misca utilizatorul in general
{   MOTION_UNKNOWN,
    MOVING,
    STATIONARY,
    IMMOBILE
};

enum ImuState {   // spune ce eveniment sau situaite a detectat IMU-ul
    IMU_NORMAL,
    IMU_IMPACT,
    IMU_POSSIBLE_FALL,
    IMU_CONFIRMED_FALL
};

struct IMUdata
{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float asvm; // Scalar Vector Magnitude pentru acceleratie
    float gsvm;

    bool fallFlag;
    MotionState motionState;
    ImuState imuState;  
};



void IMU_init();
IMUdata IMU_read();
IMUdata IMU_interpret(IMUdata data);
// bool IMU_getFallFlag();


#endif // IMU_H