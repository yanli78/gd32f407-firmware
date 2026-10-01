/**
 ****************************************************************************************************
 * @file        rtc.h
 * @version     V1.0
 * @brief       RTC 驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 正点原子
 *
 * 实验平台:    GD32F470VET6
 *
 ****************************************************************************************************
 */

#ifndef __RTC_H
#define __RTC_H

#include "Head.h"


/******************************************************************************************/

extern rtc_parameter_struct rtc_initpara;  /* RTC 参数结构体 */
uint8_t rtc_config(void);                                                       /* 初始化 RTC */

void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *ampm);    /* 获取时间 */
uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t ampm);     /* 设置时间 */
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week); /* 获取日期 */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week);  /* 设置日期 */

void rtc_set_wakeup(uint8_t wksel, uint16_t cnt);                               /* 设置自动唤醒 */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day);                /* 获取星期 */
void rtc_set_alarma(uint8_t week, uint8_t hour, uint8_t min, uint8_t sec);      /* 设置闹钟 */
uint8_t rtc_dec2bcd(uint8_t val);

/* 说明：
   - 原 rtc_show_time() 用 printf 打印时间，依赖半主机/串口重定向，已移除；
   - 原 RTC_Init() / rtc_pre_config() 只有声明没有实现，已删除。 */

#endif

















