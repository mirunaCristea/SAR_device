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
    float ax = 0.0f;
    float ay = 0.0f;
    float az = 1.0f;
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;
    float asvm = 1.0f;
    float gsvm = 0.0f;

    bool fallFlag = false;
    MotionState motionState = MOTION_UNKNOWN;
    ImuState imuState = IMU_NORMAL;
};



bool IMU_init();
bool IMU_read(IMUdata &data);
IMUdata IMU_interpret(IMUdata data);



#endif // IMU_H