#ifndef _BOOT_FLASH_H
#define _BOOT_FLASH_H

#include "Boot_Config.h"

uint8_t Boot_Flash_Erase(uint32_t addr, uint32_t size);
uint8_t Boot_Flash_Write(uint32_t addr, const uint8_t *data, uint32_t size);

#endif
