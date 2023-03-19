#ifndef __RELAY_H
#define __RELAY_H
#include "sys.h"
#include "delay.h"

void RelayControl_Init(void);
void LampControl(u8 lampid, u8 status);
#endif

