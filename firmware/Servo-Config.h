#ifndef SERVO_CONFIG
#define SERVO_CONFIG

/**
    For use internal to TLC5940-Servo, no need to include this file in your own work 
*/
#include "application.h"


struct ChannelConfig
{
    short minAngle;
    short minuSec;
    short maxAngle;
    short maxuSec;
    short initPosition;
};


struct Configuration
{
    ChannelConfig channel[16];
    
    void SetChannel(byte chan, short minAngle, short minuSec, short maxAngle, short maxuSec, short initAngle);
    
    void SetInitAngle(byte chan, short initAngle);

    Configuration();
};
