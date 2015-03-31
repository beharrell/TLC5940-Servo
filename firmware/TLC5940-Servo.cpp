#include "TLC5940-Servo.h"
#include "Servo-Config.h"

static void InvSetChannel(byte chan, short val);
static void InitTLC5940_DIO();
static void InitTLC5940_Timers();
static void ByteOut(byte data);


static bool needsXLAT = true;
static byte gsData[24];

// this makes sure that config is initialised with default values  
static Configuration config;




extern "C" void BlankSignalStarted()
{
    if (TIM_GetFlagStatus(TIM2,TIM_IT_CC1)) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
        
        if (needsXLAT)
        {
            delayMicroseconds(10);
            digitalWrite(XLAT_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(XLAT_PIN, LOW);
            needsXLAT = false;
        }
    }
}






void TLC5940Servo::SetChannelConfig(byte chan, short minAngle, short minuSec, short maxAngle, short maxuSec, short initAngle)
{
    config.SetChannel(chan, minAngle, minuSec, maxAngle, maxuSec, initAngle);
}




void TLC5940Servo::SetInitAngle(byte chan, short initAngle)
{
     config.SetInitAngle(chan,initAngle);
}




void TLC5940Servo::Init()
{
    InitTLC5940_DIO();
    
    // Write out initial position data before the TLC5940 starts PWM.
    // This way the servos will begin moving to a define position
    ClearAllChannels(); 
    for (byte i =0; i < 16; ++i)
    {
        SetChannel(i, config.channel[i].initPosition);
    }
    Update();
    
    InitTLC5940_Timers();
}




bool TLC5940Servo::Update()
{
    bool sent = !needsXLAT;
    if (!needsXLAT)
    {   // send data only when last data had been latched
        for (int i = 23; i >= 0; --i)
        {
            ByteOut(gsData[i]); // MSB first
        }
    
        needsXLAT = true;
    }
    
    return sent;
}




void TLC5940Servo::ClearAllChannels()
{
    memset(gsData, 0, 24);
}




void TLC5940Servo::SetChannel(byte chan, short angle)
{
    const short blankWidth = 80; //uSec The Blank Pulse is high during this period
    const short gsUnitPulse = 5;  //uSec
    const short countsOffEndOf50HzPwm = 96; //take 20.48 mSec to make 4096 pulses, but have a 20mSec cycle, so 96 counts are not useable
    const short blankCounts = blankWidth /gsUnitPulse;
    const short usecMin = config.channel[chan].minuSec; 
    const float uSecRange = config.channel[chan].maxuSec - usecMin;
    const float minAngle = config.channel[chan].minAngle;
    const float angleRange = config.channel[chan].maxAngle - minAngle;
    const short pulseAfterBlank = usecMin - blankWidth;
    const short baseCount = (pulseAfterBlank / gsUnitPulse) + blankCounts + countsOffEndOf50HzPwm;
    
    short uSecOffsetFromServoMin = (((float)angle - minAngle) / angleRange) * uSecRange;
    short count = baseCount + (uSecOffsetFromServoMin / gsUnitPulse);
    
    InvSetChannel(chan, count);
}




static void BitOut(bool high)
{
    digitalWrite(SIN_PIN, high ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(SCLOCK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(SCLOCK_PIN, LOW);
    digitalWrite(SIN_PIN, LOW); 
}




static void ByteOut(byte data)
{
    for (int i = 0; i < 8; ++i)
    {
        bool bit = (data & 0x80); //MSb first
        BitOut(bit);
        data = data << 1;
    }
}




// channel [0,15], value [0 - 4095]
static void SetChannel(byte channel, short value)
{
    byte baseIdx = (channel * 12) / 8;
    value &= 0x0fff; // make sure its only a 12bit value
    
    if (channel % 2 == 0)
    {
        gsData[baseIdx] = value;
        gsData[baseIdx + 1] = (gsData[baseIdx + 1] & 0xf0) | (value >> 8); // clear bottom 4 bits and assign top 4 bits of value to it
    }
    else
    {
        gsData[baseIdx] =  (gsData[baseIdx] & 0x0F) | (value << 4); // clear top 4 bits and assign low 4bits of value to it
        gsData[baseIdx + 1] = (value >> 4);
    }
}




static uint16_t PWM_FREQ = 50;

// PWM on A0 (TIM2, channel 1) represents the blank pulse
// pluse is 118uSec wide according to the scope
static void InitBlankTimer() {
  uint32_t PWM_CLOCK = 1000000;
  uint16_t TIM_ARR = (uint16_t)(PWM_CLOCK / PWM_FREQ) - 1; 

  TIM_OCInitTypeDef TIM_OCInitStructure;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

  //PWM Frequency : PWM_FREQ (Hz)
  uint16_t TIM_Prescaler = (uint16_t)(SystemCoreClock / PWM_CLOCK) - 1; 
  // TIM Channel Duty Cycle(%) = (TIM_CCR / TIM_ARR + 1) * 100
  uint16_t TIM_CCR = (uint16_t)(254 * (TIM_ARR + 1) / 255);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  pinMode(A0, AF_OUTPUT_PUSHPULL);

  // TIM clock enable
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

  // Time base configuration
  TIM_TimeBaseStructure.TIM_Period = TIM_ARR;
  TIM_TimeBaseStructure.TIM_Prescaler = TIM_Prescaler;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, & TIM_TimeBaseStructure);
  
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; 
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_Pulse = TIM_CCR;
  TIM_OC1Init(TIM2, & TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
  TIM_ARRPreloadConfig(TIM2, ENABLE);
  
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
  TIM_Cmd(TIM2, ENABLE);
}




// config timer4 chan 1 - output on D1
// GS clock period is 5uSec wide (20 Khz) - so takes 20.48 mSec to make 4096 pulses
// BLANK pulse fires every 20mSec and is 118usec wide. This means the smallest pulse we can
// represent for a servo is 118usec.
// The first 96 counts (0.48mSec ) are off the end of the 20mSec pwm so are lost by the blank signal reseting the counter
// The next 24 count (120 usec) are not used because the blank signal will hold the PWM high during the blank pulse
static void initGSTimer()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;//PIN_GSCLK; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_BaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    //uint16_t clockAdjust = 4; // seems to take 4 ticks 
    //uint16_t GSCLOCK = 4096 * PWM_FREQ * clockAdjust;
    uint16_t GSCLK_Prescaler = (uint16_t) ((SystemCoreClock / 800000) - 1); 
    //uint16_t GSCLK_Prescaler = (uint16_t) ((SystemCoreClock / GSCLOCK) - 1); 
    TIM_BaseInitStructure.TIM_Period = 1;
    TIM_BaseInitStructure.TIM_Prescaler = GSCLK_Prescaler;
    TIM_BaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_BaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_BaseInitStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM4, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);

    TIM_Cmd(TIM4, ENABLE);
}


static void InitTLC5940_DIO()
{
  pinMode(XLAT_PIN, OUTPUT);
  pinMode(BLANK_PIN, OUTPUT); 
  pinMode(SCLOCK_PIN, OUTPUT);
  pinMode(SIN_PIN, OUTPUT);
}



static void InitTLC5940_Timers()
{
  Wiring_TIM2_Interrupt_Handler = BlankSignalStarted;
  
  initGSTimer();
  InitBlankTimer();
}




// using pull up resistors to get the voltage pulse for servos, so the pulse is inverted
static void InvSetChannel(byte chan, short val)
{
    SetChannel(chan, 4096 - val);
}
