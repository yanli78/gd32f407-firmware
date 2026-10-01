/**
 ****************************************************************************************************
 * @file        myiic.h
 * @version     V1.0
 * @brief       OLED IIC 模式驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 正点原子
 *
 * 实验平台:    GD32F470VET6
 *
 ****************************************************************************************************
 */	
 
#ifndef __MYIIC_H
#define __MYIIC_H

#include <stdint.h>
#include "gd32f4xx.h"


/******************************************************************************************/
/* OLED IIC 模式引脚定义 */

#define OLED_IIC_SCL_GPIO_PORT               GPIOB
#define OLED_IIC_SCL_GPIO_PIN                GPIO_PIN_4
#define OLED_IIC_SCL_GPIO_CLK                RCU_GPIOB    /* GPIOB 时钟使能 */

#define OLED_IIC_SDA_GPIO_PORT               GPIOB
#define OLED_IIC_SDA_GPIO_PIN                GPIO_PIN_5
#define OLED_IIC_SDA_GPIO_CLK                RCU_GPIOB     /* GPIOB 时钟使能 */

/******************************************************************************************/

/* IO 操作函数 */
#define IIC_SCL(x)      do{ x ? \
                            gpio_bit_write(OLED_IIC_SCL_GPIO_PORT, OLED_IIC_SCL_GPIO_PIN, SET) : \
                            gpio_bit_write(OLED_IIC_SCL_GPIO_PORT, OLED_IIC_SCL_GPIO_PIN, RESET); \
                        }while(0)         /* SCL */ 

#define IIC_SDA(x)      do{ x ? \
                            gpio_bit_write(OLED_IIC_SDA_GPIO_PORT, OLED_IIC_SDA_GPIO_PIN, SET) : \
                            gpio_bit_write(OLED_IIC_SDA_GPIO_PORT, OLED_IIC_SDA_GPIO_PIN, RESET); \
                        }while(0)         /* SDA */

#define IIC_READ_SDA    gpio_input_bit_get(OLED_IIC_SDA_GPIO_PORT, OLED_IIC_SDA_GPIO_PIN)  /* 读取 SDA */

/******************************************************************************************/
                        
/* IIC 所有操作函数 */
void iic_init(void);                /* 初始化 IIC 的 IO 口 */
void iic_start(void);               /* 产生 IIC 起始信号 */
void iic_stop(void);                /* 产生 IIC 停止信号 */
void iic_ack(void);                 /* IIC 发送 ACK 信号 */
void iic_nack(void);                /* IIC 不发送 ACK 信号 */
uint8_t iic_wait_ack(void);         /* IIC 等待 ACK 信号 */
void iic_send_byte(uint8_t txd);    /* IIC 发送一个字节 */
uint8_t iic_read_byte(uint8_t ack); /* IIC 读取一个字节 */
void delay_us(uint32_t nus);
              
#endif












