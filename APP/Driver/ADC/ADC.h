#ifndef __GD30AD3344_H
#define __GD30AD3344_H

#include "Head.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************* 硬件配置宏定义 *************************/
// SPI 接口配置 - 默认使用 SPI0
#define AD3344_SPI_PERIPH        SPI0
#define AD3344_SPI_RCU           RCU_SPI0
#define AD3344_SPI_GPIO_RCU      RCU_GPIOA
#define AD3344_SPI_GPIO_PORT     GPIOA
#define AD3344_SPI_AF            GPIO_AF_5

// SPI 引脚定义
#define AD3344_SCK_PIN           GPIO_PIN_5
#define AD3344_MISO_PIN          GPIO_PIN_6
#define AD3344_MOSI_PIN          GPIO_PIN_7

// CS 片选引脚定义
#define AD3344_CS_RCU            RCU_GPIOA
#define AD3344_CS_GPIO_PORT      GPIOA
#define AD3344_CS_PIN            GPIO_PIN_4

/************************* 参数配置宏定义 *************************/
// 输入模式定义
#define AD3344_SINGLE_END        0x00 // 单端输入
#define AD3344_DUAL_END          0x01 // 差分输入

// 数据传输速率定义
#define AD3344_DR_6_25SPS        0x00
#define AD3344_DR_12_5SPS        0x01
#define AD3344_DR_25SPS          0x02
#define AD3344_DR_50SPS          0x03
#define AD3344_DR_100SPS         0x04
#define AD3344_DR_250SPS         0x05
#define AD3344_DR_500SPS         0x06
#define AD3344_DR_1000SPS        0x07

// PGA 增益定义（对应满量程输入范围）
#define AD3344_PGA_6_144V        0x00 // ±6.144V
#define AD3344_PGA_4_096V        0x01 // ±4.096V
#define AD3344_PGA_2_048V        0x02 // ±2.048V
#define AD3344_PGA_1_024V        0x03 // ±1.024V
#define AD3344_PGA_0_512V        0x04 // ±0.512V
#define AD3344_PGA_0_256V        0x05 // ±0.256V
#define AD3344_PGA_0_064V        0x06 // ±0.064V

/************************* 函数声明 *************************/
/**
 * @brief  初始化 GD30AD3344 ADC
 * @param  input_mode: 输入模式，AD3344_SINGLE_END 或 AD3344_DUAL_END
 * @param  channel: 通道号(0-3)
 * @param  data_rate: 数据传输速率
 * @param  pga_gain: PGA 增益
 * @retval 无
 */
void ad3344_init(uint8_t input_mode, uint8_t channel, uint8_t data_rate, uint8_t pga_gain);

/**
 * @brief  读取单次转换结果(16 位有符号整数)
 * @param  无
 * @retval ADC 原始值
 */
int16_t ad3344_read_single(void);

/**
 * @brief  启动连续转换模式
 * @param  无
 * @retval 无
 */
void ad3344_start_continuous(void);

/**
 * @brief  停止连续转换模式
 * @param  无
 * @retval 无
 */
void ad3344_stop_continuous(void);

/**
 * @brief  读取连续转换结果(16 位有符号整数)
 * @param  无
 * @retval ADC 原始值
 */
int16_t ad3344_read_continuous(void);

/**
 * @brief  将原始 ADC 值转换为电压值
 * @param  adc_value: ADC 原始值
 * @retval 电压值(V)
 */
float ad3344_to_voltage(int16_t adc_value);

/**
 * @brief  扫描全部 4 个输入通道
 * @param  voltages: 存储 4 个通道电压值的数组
 * @retval 无
 */
void ad3344_scan_all_channels(float voltages[4]);

#ifdef __cplusplus
}
#endif


#endif /* __GD30AD3344_H */

