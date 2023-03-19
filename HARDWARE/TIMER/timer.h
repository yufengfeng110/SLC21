#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

extern volatile unsigned long long FreeRTOSRunTimeTicks;

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM3_PWM_Init(u16 arr,u16 psc);
void PWM_Servicefunc(u8 ledid, u16 pwmperval);
void ConfigureTimeForRunTimeStats(void);
#endif
