#ifndef _BOOT_PROTOCOL_H
#define _BOOT_PROTOCOL_H

#include "Boot_Config.h"

void Boot_Protocol_Init(uint16_t device_id);
void Boot_Protocol_Process(void);
uint8_t Boot_Protocol_Stay(void);
uint8_t Boot_Protocol_UpgradeReady(void);
uint32_t Boot_Protocol_UpgradeSize(void);
uint8_t Boot_Protocol_ExecuteUpgrade(void);

#endif
