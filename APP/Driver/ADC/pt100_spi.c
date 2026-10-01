#include "pt100_spi.h" 
#include "systick.h"

static float ad3344_fsr = 4.096f;
static uint8_t ad3344_input_mode = PT100_AD3344_SINGLE_END;
static uint8_t ad3344_channel = 0U;
static uint8_t ad3344_data_rate = PT100_AD3344_DR_6_25SPS;
static PT100_AD3344_PGA_TypeDef ad3344_pga_gain = PT100_AD3344_PGA_4_096V;

#define PT100_AD3344_OS_SINGLE         0x8000U
#define PT100_AD3344_MODE_SINGLE       0x0100U
#define PT100_AD3344_PULL_UP_EN        0x0008U
#define PT100_AD3344_NOP_VALID         0x0002U
#define PT100_AD3344_RESERVED_VALUE    0x0001U

static uint8_t spi_read_write_byte(uint8_t data)
{
    while(RESET == spi_i2s_flag_get(PT100_AD3344_SPI_PERIPH, SPI_FLAG_TBE));
    spi_i2s_data_transmit(PT100_AD3344_SPI_PERIPH, data);
    while(RESET == spi_i2s_flag_get(PT100_AD3344_SPI_PERIPH, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(PT100_AD3344_SPI_PERIPH);
}

static uint16_t spi_read_write_word(uint16_t data)
{
    uint16_t receive_data;

    receive_data = (uint16_t)spi_read_write_byte((uint8_t)(data >> 8)) << 8;
    receive_data |= spi_read_write_byte((uint8_t)data);
    return receive_data;
}

static uint16_t ad3344_build_config(uint8_t input_mode, uint8_t channel, uint8_t data_rate, PT100_AD3344_PGA_TypeDef pga_gain)
{
    uint16_t config = PT100_AD3344_OS_SINGLE | PT100_AD3344_MODE_SINGLE |
                      PT100_AD3344_PULL_UP_EN | PT100_AD3344_NOP_VALID |
                      PT100_AD3344_RESERVED_VALUE;

    if(input_mode == PT100_AD3344_SINGLE_END){
        config |= (uint16_t)(0x4000U + ((channel & 0x03U) << 12));
    }else{
        config |= (uint16_t)((channel & 0x03U) << 12);
    }

    config |= (uint16_t)((pga_gain & 0x07U) << 9);
    config |= (uint16_t)((data_rate & 0x07U) << 5);
    return config;
}

static int16_t ad3344_transfer_config(uint16_t config)
{
    uint16_t val;

    PT100_AD3344_CS_LOW();
    val = spi_read_write_word(config);
    PT100_AD3344_CS_HIGH();

    return (int16_t)val;
}

static void ad3344_wait_conversion(void)
{
    switch(ad3344_data_rate){
        case PT100_AD3344_DR_6_25SPS: delay_1ms(170); break;
        case PT100_AD3344_DR_12_5SPS: delay_1ms(90); break;
        case PT100_AD3344_DR_25SPS: delay_1ms(45); break;
        case PT100_AD3344_DR_50SPS: delay_1ms(25); break;
        case PT100_AD3344_DR_100SPS: delay_1ms(15); break;
        case PT100_AD3344_DR_250SPS: delay_1ms(6); break;
        case PT100_AD3344_DR_500SPS: delay_1ms(4); break;
        case PT100_AD3344_DR_1000SPS: delay_1ms(3); break;
        default: delay_1ms(170); break;
    }
}

static void ad3344_write_config(uint16_t config)
{
    (void)ad3344_transfer_config(config);
}

static float pt100_resistance_to_temperature(float resistance)
{
    float low = -50.0f;
    float high = 150.0f;
    uint8_t i;

    for(i = 0U; i < 24U; i++){
        float mid = (low + high) * 0.5f;
        float mid2 = mid * mid;
        float rt;

        if(mid < 0.0f){
            rt = 100.0f * (1.0f + 3.9083e-3f * mid - 5.775e-7f * mid2 - 4.183e-12f * (mid - 100.0f) * mid2 * mid);
        }else{
            rt = 100.0f * (1.0f + 3.9083e-3f * mid - 5.775e-7f * mid2);
        }

        if(rt < resistance) low = mid;
        else high = mid;
    }

    return (low + high) * 0.5f;
}

static float pt100_compensate_resistance(float measured_resistance)
{
    static const float measured_table[] = {80.306f, 101.336f, 112.900f, 114.100f, 115.061f, 123.896f, 130.212f, 140.929f, 142.138f, 142.285f};
    static const float actual_table[] = {80.6f, 100.0f, 111.85f, 111.85f, 115.0f, 124.0f, 130.0f, 140.0f, 150.0f, 154.0f};
    uint8_t i;

    if(measured_resistance <= measured_table[0]) return actual_table[0];

    for(i = 1U; i < (uint8_t)(sizeof(measured_table) / sizeof(measured_table[0])); i++){
        if(measured_resistance <= measured_table[i]){
            float ratio = (measured_resistance - measured_table[i - 1U]) / (measured_table[i] - measured_table[i - 1U]);
            return actual_table[i - 1U] + ratio * (actual_table[i] - actual_table[i - 1U]);
        }
    }

    return actual_table[(sizeof(actual_table) / sizeof(actual_table[0])) - 1U];
}

static void ad3344_spi_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(PT100_AD3344_SPI_RCU);
    rcu_periph_clock_enable(PT100_AD3344_SPI_GPIO_RCU);
    rcu_periph_clock_enable(PT100_AD3344_CS_RCU);

    gpio_af_set(PT100_AD3344_SPI_GPIO_PORT, PT100_AD3344_SPI_AF, PT100_AD3344_SCK_PIN);
		gpio_af_set(PT100_AD3344_SPI_GPIO_PORT, PT100_AD3344_SPI_AF, PT100_AD3344_MISO_PIN);
		gpio_af_set(PT100_AD3344_SPI_GPIO_PORT, PT100_AD3344_SPI_AF, PT100_AD3344_MOSI_PIN);
    gpio_mode_set(PT100_AD3344_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  PT100_AD3344_SCK_PIN | PT100_AD3344_MISO_PIN | PT100_AD3344_MOSI_PIN);
    gpio_output_options_set(PT100_AD3344_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            PT100_AD3344_SCK_PIN | PT100_AD3344_MISO_PIN | PT100_AD3344_MOSI_PIN);

    gpio_mode_set(PT100_AD3344_CS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PT100_AD3344_CS_PIN);
    gpio_output_options_set(PT100_AD3344_CS_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PT100_AD3344_CS_PIN);
    PT100_AD3344_CS_HIGH();

    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_256; 
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(PT100_AD3344_SPI_PERIPH, &spi_init_struct);
    spi_enable(PT100_AD3344_SPI_PERIPH);
}

// 函数增加 pt100_ 前缀
void pt100_ad3344_init(uint8_t input_mode, uint8_t channel, uint8_t data_rate, PT100_AD3344_PGA_TypeDef pga_gain)
{
    static uint8_t spi_inited = 0;
    if(!spi_inited){
        ad3344_spi_init();
        spi_inited = 1;
    }

    ad3344_input_mode = input_mode;
    ad3344_channel = channel & 0x03U;
    ad3344_data_rate = data_rate & 0x07U;
    ad3344_pga_gain = pga_gain;

    switch(pga_gain){
        case PT100_AD3344_PGA_6_144V: ad3344_fsr = 6.144f; break;
        case PT100_AD3344_PGA_4_096V: ad3344_fsr = 4.096f; break;
        case PT100_AD3344_PGA_2_048V: ad3344_fsr = 2.048f; break;
        case PT100_AD3344_PGA_1_024V: ad3344_fsr = 1.024f; break;
        case PT100_AD3344_PGA_0_512V: ad3344_fsr = 0.512f; break;
        case PT100_AD3344_PGA_0_256V: ad3344_fsr = 0.256f; break;
        case PT100_AD3344_PGA_0_064V: ad3344_fsr = 0.064f; break;
        default: ad3344_fsr = 4.096f; break;
    }

    ad3344_write_config(ad3344_build_config(ad3344_input_mode, ad3344_channel, ad3344_data_rate, ad3344_pga_gain));
}

int16_t pt100_ad3344_read_single(void)
{
    uint16_t config = ad3344_build_config(ad3344_input_mode, ad3344_channel, ad3344_data_rate, ad3344_pga_gain);

    ad3344_write_config(config);
    ad3344_wait_conversion();
    return ad3344_transfer_config(config);
}

float pt100_ad3344_to_voltage(int16_t adc_value)
{
    return (float)adc_value * ad3344_fsr / 32768.0f;
}

float pt100_ad3344_read_resistance(uint8_t channel, float ref_resistor)
{
    (void)ref_resistor;

    if((ad3344_input_mode != PT100_AD3344_SINGLE_END) || ((channel & 0x03U) != ad3344_channel)){
        pt100_ad3344_init(PT100_AD3344_SINGLE_END, channel, ad3344_data_rate, ad3344_pga_gain);
    }

    (void)pt100_ad3344_read_single();
    float v0 = pt100_ad3344_to_voltage(pt100_ad3344_read_single());
    float v1 = pt100_ad3344_to_voltage(pt100_ad3344_read_single());
    float v2 = pt100_ad3344_to_voltage(pt100_ad3344_read_single());
    if(v0 > v1){ float t = v0; v0 = v1; v1 = t; }
    if(v1 > v2){ float t = v1; v1 = v2; v2 = t; }
    if(v0 > v1){ float t = v0; v0 = v1; v1 = t; }
    float v = v1;
    if(v >= 3.29f) return 999999.0f;
    if(v <= 0.01f) return 0.0f;
    return 13.7488f * v + 97.6430f;
}

float pt100_read_temperature(uint8_t channel, float ref_resistor)
{
    float Rpt = pt100_ad3344_read_resistance(channel, ref_resistor);
    return pt100_resistance_to_temperature(pt100_compensate_resistance(Rpt));
}

