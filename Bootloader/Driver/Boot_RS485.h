#ifndef _BOOT_RS485_H
#define _BOOT_RS485_H

#include "Boot_Config.h"

void Boot_RS485_Init(uint32_t baudrate);
void Boot_RS485_SendString(const char *str);
void Boot_RS485_StartRaw(void);
uint8_t Boot_RS485_RawFinished(void);
const volatile uint8_t *Boot_RS485_RawBuf(void);
uint32_t Boot_RS485_RawLen(void);
uint8_t Boot_RS485_GetFrame(uint8_t *ascii, uint16_t *ascii_len);

#endif
