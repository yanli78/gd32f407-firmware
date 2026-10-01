#ifndef _BOOT_TIMER_H
#define _BOOT_TIMER_H

#include "Boot_Config.h"

void Boot_Timer_Init(void);
void Boot_DelayMs(uint32_t ms);
uint32_t Boot_Ms(void);
uint32_t Boot_S(void);

#endif
