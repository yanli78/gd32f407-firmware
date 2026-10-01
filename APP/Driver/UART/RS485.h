#ifndef _RS485_H
#define _RS485_H

#include "Head.h"

#define RS485_RX_BUF_LEN 256
#define RS485_RAW_BUF_LEN 131072U

extern volatile uint8_t rs485_rx_buf[RS485_RX_BUF_LEN];
extern volatile uint16_t rs485_rx_index;
extern volatile uint8_t rs485_rx_done;
extern volatile uint8_t rs485_raw_mode;
extern volatile uint8_t rs485_raw_buf[RS485_RAW_BUF_LEN];
extern volatile uint32_t rs485_raw_len;
extern volatile uint8_t rs485_raw_done;
extern volatile uint32_t rs485_raw_last_s;

void RS485_Config(uint32_t baudrate);
void RS485_SetBaudrate(uint32_t baudrate);
void RS485_SendChar(uint8_t ch);
void RS485_SendString(const char *str);
void RS485_IRQHandler(void);

#endif
