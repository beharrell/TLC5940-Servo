#ifndef TLC5940SERVO_H
#define TLC5940SERVO_H

#include "application.h"

// Pins used to control the TLC5940
#define BLANK_PIN A0
#define GSCLOCK_PIN D1
#define XLAT_PIN D2
#define SCLOCK_PIN D3
#define SIN_PIN D4

// Values used to provide the default initialisation of the Servos
#define ANGLE_MIN 0
#define USEC_MIN 600
#define ANGLE_MAX 180
#define USEC_MAX 2400 
#define INIT_ANGLE  90


#ifdef __cplusplus
extern "C" {
#endif
// interrupt we are going to override to provide the XLAT pulse
extern "C" void (*Wiring_TIM2_Interrupt_Handler)(void);

#ifdef __cplusplus
}
#endif


/*
The following is an interface for the TLC5940 allowing it to be used to drive upto 16 servos.
Data sheet may be found at www.ti.com/lit/ds/symlink/tlc5940.pdf
The implementation makes use of timer4 to provide the GS clock and timer2 to provide the blank pulse
*/
class TLC5940Servo
{
    public:
        // intialise and start PWM - any per channel configuration should be done before this
        static void Init();
        
        // Override the default initialisation of the servo
        static void SetChannelConfig(byte chan, short minAngle, short minuSec, short maxAngle, short maxuSec, short initAngle);
        
        // Override the default initial position of the servo 
        static void SetInitAngle(byte chan, short initAngle);
        
        // Set all channel values to 0 (this is NOT the same as setting the angle to 0!!)
        static void ClearAllChannels();
        
        // set a channel to the desired angle
        // chan in range 0 to 15
        // angle in the range of the min and max angles you have defined for that channel (0 and 180 by default)
        static void SetChannel(byte chan, short angle);
        
        // Write out all channel values to the TLC5940 - will return false if the last update (XLAT pulse) has not
        // yet completed.
        static bool Update();
};

#endif