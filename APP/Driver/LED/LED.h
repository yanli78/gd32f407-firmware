#ifndef __LED_H
#define __LED_H

#include "Head.h"

/************************* LED 引脚定义 *************************/
#define LED1_Port1  RCU_GPIOE
#define LED1_Port   GPIOE
#define LED1_Pin    GPIO_PIN_3

#define LED2_Port1  RCU_GPIOE
#define LED2_Port   GPIOE
#define LED2_Pin    GPIO_PIN_5

/************************* 函数声明 *************************/
void LED_Init(void);     /* LED 初始化 */

#endif

/****************************End*****************************/
