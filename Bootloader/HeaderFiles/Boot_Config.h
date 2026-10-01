#ifndef _BOOT_CONFIG_H
#define _BOOT_CONFIG_H

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>

#define BOOT_APP_ADDR          0x08011000U
#define BOOT_APP_END           0x08031000U
#define BOOT_FLAG_ADDR         0x08010000U
#define BOOT_TEMP_ADDR         0x08051000U
#define BOOT_SLOT_SIZE         0x00020000U
#define BOOT_FLAG_MAGIC        0x424F4F54U
#define BOOT_UPGRADE_MAGIC     0x5AA5C33CU
#define BOOT_DEVICE_ID         0x0008U
#define BOOT_VERSION           0x02U
#define BOOT_BAUDRATE          19200U
#define RS485_RX_BUF_LEN       256U
#define RS485_RAW_BUF_LEN      131072U

typedef struct {
    uint32_t magic;
    uint16_t device_id;
    uint16_t baud_code;
    uint32_t baudrate;
} BootFlagTypeDef;

#endif
