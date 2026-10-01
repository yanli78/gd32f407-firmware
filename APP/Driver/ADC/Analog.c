#include "Head.h"

/* 参考电压与 12 位 ADC/DAC 满量程 */
#define ANALOG_VREF                 3.3f
#define ANALOG_FULL_SCALE           4095.0f
/* CH0 下限：输入开路/短路时 ADC 读数会掉到 0 附近，
   按原设计把低于此值的读数钳到 1.0V（保持原有行为） */
#define ANALOG_CH0_MIN_VOLTAGE      0.05f
#define ANALOG_CH0_CLAMP_VOLTAGE    1.0f
/* ADC 转换完成等待超时（主循环轮询次数），避免硬件异常时死等 */
#define ANALOG_ADC_TIMEOUT          100000U

#define ANALOG_CH0_PORT             GPIOA
#define ANALOG_CH0_PIN              GPIO_PIN_0
#define ANALOG_CH0_RCU              RCU_GPIOA
#define ANALOG_CH0_ADC_CHANNEL      ADC_CHANNEL_0

/* CH1 对外表现为“DAC 设定值回读”，PC1 按模拟输入配置备用 */
#define ANALOG_CH1_PORT             GPIOC
#define ANALOG_CH1_PIN              GPIO_PIN_1
#define ANALOG_CH1_RCU              RCU_GPIOC
#define ANALOG_CH1_ADC_CHANNEL      ADC_CHANNEL_11

/* DAC 输出引脚 PA4 */
#define ANALOG_DAC_PORT             GPIOA
#define ANALOG_DAC_PIN              GPIO_PIN_4

static uint16_t analog_dac_value = 0U;

/**
 * @brief  单次启动并读取 ADC0 规则通道
 * @param  channel: ADC 通道号
 * @retval 12 位转换结果；超时返回 0
 */
static uint16_t analog_adc_read(uint8_t channel)
{
    uint32_t timeout = ANALOG_ADC_TIMEOUT;

    adc_routine_channel_config(ADC0, 0U, channel, ADC_SAMPLETIME_480);
    adc_software_trigger_enable(ADC0, ADC_ROUTINE_CHANNEL);
    while(RESET == adc_flag_get(ADC0, ADC_FLAG_EOC)) {
        if(timeout-- == 0U) {
            return 0U;              /* 超时退出，不再死等 */
        }
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
    /* PA4 为 DAC0_OUT0 输出引脚 */
    gpio_mode_set(ANALOG_DAC_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ANALOG_DAC_PIN);

    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, DISABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADC0, ADC_ROUTINE_CHANNEL, 1U);
    adc_external_trigger_config(ADC0, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_enable(ADC0);
    delay_1ms(1U);                  /* 等待 ADC 稳定后再校准 */
    adc_calibration_enable(ADC0);

    dac_deinit(DAC0);
    dac_output_buffer_enable(DAC0, DAC_OUT0);
    dac_enable(DAC0, DAC_OUT0);
    Analog_SetDac(0U);
}

/** @brief 读取 CH0（PA0 / ADC0_IN0）电压，单位 V */
float Analog_ReadCH0(void)
{
    float voltage = ((float)analog_adc_read(ANALOG_CH0_ADC_CHANNEL) * ANALOG_VREF) / ANALOG_FULL_SCALE;
    if(voltage < ANALOG_CH0_MIN_VOLTAGE) voltage = ANALOG_CH0_CLAMP_VOLTAGE;
    return voltage;
}

/** @brief 读取 CH1：返回当前 DAC 设定值对应的电压，单位 V */
float Analog_ReadCH1(void)
{
    return ((float)analog_dac_value * ANALOG_VREF) / ANALOG_FULL_SCALE;
}

/** @brief 设置 DAC 输出码值（0~4095） */
void Analog_SetDac(uint16_t value)
{
    analog_dac_value = value & 0x0FFFU;
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, analog_dac_value);
}

/** @brief 读取当前 DAC 输出码值 */
uint16_t Analog_GetDac(void)
{
    return analog_dac_value;
}
