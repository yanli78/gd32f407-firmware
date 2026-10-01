#ifndef _HEAD_H
#define _HEAD_H






#include "stdio.h"
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "gd32f4xx_rtc.h"
#include "systick.h"
#include "gd32f4xx_it.h"
#include <stdio.h>


#include "Tim.h"
#include "LED.h"
#include "KEY.h"
#include "USART0.h"
#include "RS485.h"
#include "Protocol.h"
#include "myiic.h"

#include "sys.h"
#include "ADC.h"
#include "Analog.h"
#include "pt100_spi.h"
#include "OLED.h"
#include "RTC.h"
#include "SPI_FLASH.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "fun.h"
#include <math.h>



extern uint32_t s;
extern int adc_value;
extern FATFS fs;
extern FIL fdst;
extern UINT br;
extern FRESULT result;
extern uint8_t buffer[128];
extern StartState start_state;

#endif

