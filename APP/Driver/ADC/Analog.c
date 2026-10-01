#include "Head.h"

#define ANALOG_CH0_PORT             GPIOA
#define ANALOG_CH0_PIN              GPIO_PIN_0
#define ANALOG_CH0_RCU              RCU_GPIOA
#define ANALOG_CH0_ADC_CHANNEL      ADC_CHANNEL_0

#define ANALOG_CH1_PORT             GPIOC
#define ANALOG_CH1_PIN              GPIO_PIN_1
#define ANALOG_CH1_RCU              RCU_GPIOC
#define ANALOG_CH1_ADC_CHANNEL      ADC_CHANNEL_11

static uint16_t analog_dac_value = 0U;

static uint16_t analog_adc_read(uint8_t channel)
{
    adc_routine_channel_config(ADC0, 0U, channel, ADC_SAMPLETIME_480);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    while(RESET == adc_flag_get(ADC0, ADC_FLAG_EOC)) {
    }
    adc_flag_clear(ADC0, ADC_FLAG_EOC);
    return adc_routine_data_read(ADC0);
}

void Analog_Init(void)
{
    rcu_periph_clock_enable(ANALOG_CH0_RCU);
    rcu_periph_clock_enable(ANALOG_CH1_RCU);
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(ANALOG_CH0_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ANALOG_CH0_PIN);
    gpio_mode_set(ANALOG_CH1_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ANALOG_CH1_PIN);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);

    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1U);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(ADC0);
    delay_1ms(1U);
    adc_calibration_enable(ADC0);

    dac_deinit(DAC0);
    dac_output_buffer_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT0);
    Analog_SetDac(0U);
}

float Analog_ReadCH0(void)
{
    float voltage = ((float)analog_adc_read(ANALOG_CH0_ADC_CHANNEL) * 3.3f) / 4095.0f;
    if(voltage < 0.05f) voltage = 1.0f;
    return voltage;
}

float Analog_ReadCH1(void)
{
    return ((float)analog_dac_value * 3.3f) / 4095.0f;
}

void Analog_SetDac(uint16_t value)
{
    analog_dac_value = value & 0x0FFFU;
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, analog_dac_value);
}

uint16_t Analog_GetDac(void)
{
    return analog_dac_value;
}
