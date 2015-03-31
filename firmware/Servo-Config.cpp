#include "Servo-Config.h"
#include "TLC5940-Servo.h"



    
void Configuration::SetChannel(byte chan, short minAngle, short minuSec, short maxAngle, short maxuSec, short initAngle)
{
    channel[chan].minAngle = minAngle;    
    channel[chan].minuSec = minuSec;
    channel[chan].maxAngle = maxAngle;
    channel[chan].maxuSec = maxuSec;
    channel[chan].initPosition = initAngle;
}
    


    
void Configuration::SetInitAngle(byte chan, short initAngle)
{
    channel[chan].initPosition = initAngle;
}




Configuration::Configuration()
{
    for (byte i = 0; i < 16; ++i)
    {
        SetChannel(i, ANGLE_MIN, USEC_MIN, ANGLE_MAX, USEC_MAX, INIT_ANGLE);
    }
}