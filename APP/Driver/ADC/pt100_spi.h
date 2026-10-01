#ifndef PT100_SPI_H
#define PT100_SPI_H

#include "Head.h"

// SPI 配置
#define PT100_AD3344_SPI_PERIPH        SPI0
#define PT100_AD3344_SPI_RCU           RCU_SPI0
#define PT100_AD3344_SPI_GPIO_PORT     GPIOA
#define PT100_AD3344_SPI_GPIO_RCU      RCU_GPIOA
#define PT100_AD3344_SCK_PIN           GPIO_PIN_5
#define PT100_AD3344_MISO_PIN          GPIO_PIN_6
#define PT100_AD3344_MOSI_PIN          GPIO_PIN_7
#define PT100_AD3344_SPI_AF            GPIO_AF_5

// CS 引脚
#define PT100_AD3344_CS_GPIO_PORT      GPIOB
#define PT100_AD3344_CS_PIN            GPIO_PIN_6
#define PT100_AD3344_CS_RCU            RCU_GPIOB

// ADC 配置
#define PT100_AD3344_SINGLE_END        0x01
#define PT100_AD3344_DUAL_END          0x00

#define PT100_AD3344_DR_6_25SPS        0x00
#define PT100_AD3344_DR_12_5SPS        0x01
#define PT100_AD3344_DR_25SPS          0x02
#define PT100_AD3344_DR_50SPS          0x03
#define PT100_AD3344_DR_100SPS         0x04
#define PT100_AD3344_DR_250SPS         0x05
#define PT100_AD3344_DR_500SPS         0x06
#define PT100_AD3344_DR_1000SPS        0x07

#define PT100_AD3344_CS_LOW()          gpio_bit_reset(PT100_AD3344_CS_GPIO_PORT, PT100_AD3344_CS_PIN)
#define PT100_AD3344_CS_HIGH()         gpio_bit_set(PT100_AD3344_CS_GPIO_PORT, PT100_AD3344_CS_PIN)

// PGA 档位
typedef enum {
    PT100_AD3344_PGA_6_144V,
    PT100_AD3344_PGA_4_096V,
    PT100_AD3344_PGA_2_048V,
    PT100_AD3344_PGA_1_024V,
    PT100_AD3344_PGA_0_512V,
    PT100_AD3344_PGA_0_256V,
    PT100_AD3344_PGA_0_064V
} PT100_AD3344_PGA_TypeDef;

void pt100_ad3344_init(uint8_t input_mode, uint8_t channel, uint8_t data_rate, PT100_AD3344_PGA_TypeDef pga_gain);
int16_t pt100_ad3344_read_single(void);
float pt100_ad3344_to_voltage(int16_t adc_value);
float pt100_ad3344_read_resistance(uint8_t channel, float ref_resistor);
float pt100_read_temperature(uint8_t channel, float ref_resistor);

#endif 
