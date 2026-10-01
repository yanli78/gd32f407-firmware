#include "AppService.h"

static uint32_t led_shangci_s = 0U;
static uint8_t led_zhuangtai = 0U;

void App_Xianshi_Zhuangtai(const char *status)
{
    oled_clear();
    oled_show_string(0, 0, "2026413756", 16);
    oled_show_string(0, 16, status, 16);
    oled_refresh_gram();
}

void App_Led_Chuli(uint8_t auto_report)
{
    if((s - led_shangci_s) >= 1U) {
        led_shangci_s = s;
        led_zhuangtai ^= 1U;
        if(led_zhuangtai != 0U) gpio_bit_set(LED1_Port, LED1_Pin);
        else gpio_bit_reset(LED1_Port, LED1_Pin);
    }

    if(auto_report != 0U) gpio_bit_set(LED2_Port, LED2_Pin);
    else gpio_bit_reset(LED2_Port, LED2_Pin);
}

void App_Shui10s(uint32_t baudrate)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    rtc_wakeup_disable();
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);
    rtc_wakeup_timer_set(9U);
    rtc_flag_clear(RTC_FLAG_WT);
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_flag_clear(EXTI_22);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_WKUP_IRQn, 1U, 1U);
    rtc_wakeup_enable();
    usart_interrupt_disable(USART1, USART_INT_RBNE);
    timer_disable(TIMER2);
    SysTick->CTRL = 0U;
    __DSB();
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_DISABLE, WFI_CMD);
    __ISB();
    rtc_wakeup_disable();
    rtc_flag_clear(RTC_FLAG_WT);
    exti_flag_clear(EXTI_22);
    SystemInit();
    SystemCoreClockUpdate();
    systick_config();
    RS485_Config(baudrate);
    Tim_Init();
    s += 10U;
}

float App_Du_CH0(float ratio)
{
    return Analog_ReadCH0() * ratio;
}

float App_Du_CH1(float ratio)
{
    return Analog_ReadCH1() * ratio;
}

float App_Du_CH2(void)
{
    return pt100_read_temperature(0, 100.0f);
}

void App_Set_Dac(uint16_t value)
{
    Analog_SetDac(value);
}
