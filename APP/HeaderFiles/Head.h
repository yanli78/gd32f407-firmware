#ifndef _HEAD_H
#define _HEAD_H

/* ---------------------------------------------------------------------------
 * 应用层公共头文件：集中引入标准库与各驱动模块，供 APP 下的 .c 使用。
 * 各驱动头文件也包含本文件，靠 include guard 保证只展开一次。
 * ------------------------------------------------------------------------- */

/************************* 标准库 *************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/************************* 芯片外设库 *************************/
#include "gd32f4xx.h"
#include "gd32f4xx_rtc.h"

/************************* 用户模块 *************************/
#include "systick.h"
#include "gd32f4xx_it.h"
#include "Tim.h"
#include "LED.h"
#include "RS485.h"
#include "Protocol.h"
#include "myiic.h"
#include "sys.h"
#include "Analog.h"
#include "pt100_spi.h"
#include "OLED.h"
#include "RTC.h"
#include "SPI_FLASH.h"
#include "fun.h"
#include "AppService.h"

/* 系统秒计数，由 TIMER2 更新中断递增（定义见 Tim.c） */
extern volatile uint32_t s;

#endif
