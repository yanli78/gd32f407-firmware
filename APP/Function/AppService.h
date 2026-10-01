#ifndef _APP_SERVICE_H
#define _APP_SERVICE_H

#include "Head.h"

void App_Xianshi_Zhuangtai(const char *status);
void App_Led_Chuli(uint8_t auto_report);
void App_Shui10s(uint32_t baudrate);
float App_Du_CH0(float ratio);
float App_Du_CH1(float ratio);
float App_Du_CH2(void);
void App_Set_Dac(uint16_t value);

#endif
