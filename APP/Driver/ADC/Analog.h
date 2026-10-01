#ifndef _ANALOG_H
#define _ANALOG_H

#include <stdint.h>

void Analog_Init(void);
float Analog_ReadCH0(void);
float Analog_ReadCH1(void);
void Analog_SetDac(uint16_t value);
uint16_t Analog_GetDac(void);

#endif
