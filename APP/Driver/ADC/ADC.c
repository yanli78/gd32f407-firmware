#include "ADC.h"
#include "systick.h"

#define AD3344_CS_LOW()  gpio_bit_reset(AD3344_CS_GPIO_PORT, AD3344_CS_PIN)
#define AD3344_CS_HIGH() gpio_bit_set(AD3344_CS_GPIO_PORT, AD3344_CS_PIN)

#define AD3344_CMD_WRITE_CONFIG  0x01
#define AD3344_CMD_READ_DATA     0x02
#define AD3344_CMD_START_CONT    0x03
#define AD3344_CMD_STOP_CONT     0x04

static float ad3344_fsr = 4.096f;

static uint8_t spi_read_write_byte(uint8_t data)
{
    while(RESET == spi_i2s_flag_get(AD3344_SPI_PERIPH, SPI_FLAG_TBE));
    spi_i2s_data_transmit(AD3344_SPI_PERIPH, data);
    while(RESET == spi_i2s_flag_get(AD3344_SPI_PERIPH, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(AD3344_SPI_PERIPH);
}

static void ad3344_spi_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(AD3344_SPI_RCU);
    rcu_periph_clock_enable(AD3344_SPI_GPIO_RCU);
    rcu_periph_clock_enable(AD3344_CS_RCU);

    gpio_af_set(AD3344_SPI_GPIO_PORT, AD3344_SPI_AF,
                AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);
    gpio_mode_set(AD3344_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);
    gpio_output_options_set(AD3344_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);

    gpio_mode_set(AD3344_CS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AD3344_CS_PIN);
    gpio_output_options_set(AD3344_CS_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_CS_PIN);
    AD3344_CS_HIGH();

    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_64;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(AD3344_SPI_PERIPH, &spi_init_struct);

    spi_enable(AD3344_SPI_PERIPH);
}
//修改配置寄存器位定义
void ad3344_init(uint8_t input_mode, uint8_t channel, uint8_t data_rate, uint8_t pga_gain)
{
    uint8_t config_reg = 0;

    static uint8_t spi_inited = 0;
    if(!spi_inited){
        ad3344_spi_init();
        spi_inited = 1;
    }

    config_reg |= (pga_gain & 0x07) << 5;    // 增益 bit7-5
    config_reg |= (channel & 0x03) << 3;     // 通道 bit4-3
    config_reg |= (input_mode & 0x01) << 2;  // 输入模式 bit2
    config_reg |= (data_rate & 0x03);        // 速率 bit1-0

    switch(pga_gain){
        case AD3344_PGA_6_144V: ad3344_fsr = 6.144f; break;
        case AD3344_PGA_4_096V: ad3344_fsr = 4.096f; break;
        case AD3344_PGA_2_048V: ad3344_fsr = 2.048f; break;
        case AD3344_PGA_1_024V: ad3344_fsr = 1.024f; break;
        case AD3344_PGA_0_512V: ad3344_fsr = 0.512f; break;
        case AD3344_PGA_0_256V: ad3344_fsr = 0.256f; break;
        case AD3344_PGA_0_064V: ad3344_fsr = 0.064f; break;
        default: ad3344_fsr = 4.096f; break;
    }

    AD3344_CS_LOW();
    spi_read_write_byte(AD3344_CMD_WRITE_CONFIG);
    spi_read_write_byte(config_reg);
    AD3344_CS_HIGH();

    delay_1ms(2);
}

int16_t ad3344_read_single(void)
{
    uint8_t h, l;
    int16_t val;

    AD3344_CS_LOW();
    spi_read_write_byte(AD3344_CMD_READ_DATA);
    h = spi_read_write_byte(0x00);
    l = spi_read_write_byte(0x00);
    AD3344_CS_HIGH();

    val = (h << 8) | l;

    // 符号扩展（16 位补码）
    if(val > 32767) val -= 65536;

    delay_1ms(1);
    return val;
}

void ad3344_start_continuous(void)
{
    AD3344_CS_LOW();
    spi_read_write_byte(AD3344_CMD_START_CONT);
    AD3344_CS_HIGH();
    delay_1ms(1);
}

void ad3344_stop_continuous(void)
{
    AD3344_CS_LOW();
    spi_read_write_byte(AD3344_CMD_STOP_CONT);
    AD3344_CS_HIGH();
    delay_1ms(1);
}

//修改连续转换读取流程 
int16_t ad3344_read_continuous(void)
{
    uint8_t h,l;
    int16_t val;

    AD3344_CS_LOW();
    spi_read_write_byte(AD3344_CMD_READ_DATA);
    h = spi_read_write_byte(0x00);
    l = spi_read_write_byte(0x00);
    AD3344_CS_HIGH();

    val = (h<<8)|l;
    if(val>32767) val-=65536;
    return val;
}

float ad3344_to_voltage(int16_t adc_value)
{
    return (float)adc_value * ad3344_fsr / 32768.0f;
}

void ad3344_scan_all_channels(float voltages[4])
{
    uint8_t i;
    for(i=0; i<4; i++){
        ad3344_init(AD3344_SINGLE_END, i, AD3344_DR_1000SPS, AD3344_PGA_4_096V);
        voltages[i] = ad3344_to_voltage(ad3344_read_single());
    }
}

//计算未知电阻（核心函数）
// 使用分压法计算：上拉 10K，测未知电阻
float ad3344_read_resistance(uint8_t channel, float ref_resistor)
{
    ad3344_init(AD3344_SINGLE_END, channel, AD3344_DR_1000SPS, AD3344_PGA_4_096V);
    float v = ad3344_to_voltage(ad3344_read_single());

    // 分压公式：V = 3.3V * R / (R + Rref)
    // 推导出：R = Rref * V / (3.3f - V)
    if(v >= 3.29f) return 999999.0f; // 开路
    if(v <= 0.01f) return 0.0f;      // 短路

    return ref_resistor * v / (3.3f - v);
}
