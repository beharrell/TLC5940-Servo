#include "TLC5940-Servo/TLC5940-Servo.h"


void setup()
{
    TLC5940Servo::Init();
}

int angle = 0;
void loop()
{
	angle = angle >= 180 ? 0 : angle + 1;
    TLC5940Servo::SetChannel(0, angle);
	TLC5940Servo::Update();
	delay(500); 
}