#include "AppService.h"

/* LED1 上一次翻转时对应的系统秒计数 */
static uint32_t led_shangci_s = 0U;
/* LED1 当前电平状态 */
static uint8_t led_zhuangtai = 0U;

/**
 * @brief  在 OLED 上显示工作状态
 * @param  status: 状态字符串（最多 16 个字符宽，ASCII）
 * @note   原来先 oled_clear()（清显存并整屏刷新）再刷新一次，
 *         一次状态切换要在 IIC 上写两遍整屏（每遍约 20ms）；
 *         这里改成只清显存，最后一次刷新写出，省掉一次整屏传输。
 */
void App_Xianshi_Zhuangtai(const char *status)
{
    oled_clear_gram();
    oled_show_string(0, 0, "2026413756", 16);
    oled_show_string(0, 16, status, 16);
    oled_refresh_gram();
}

/**
 * @brief  心跳 / 状态灯处理，主循环里周期调用
 * @param  auto_report: 非 0 表示正在自动上报，点亮 LED2
 * @note   LED1 每秒翻转一次（2 秒一个闪烁周期）；
 *         s 每秒 +1，用无符号相减，计数器回绕也不会出错。
 */
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

/**
 * @brief  进入深度睡眠约 10 秒（RTC 自动唤醒）后恢复运行
 * @param  baudrate: 唤醒后需要恢复的 RS485 波特率
 * @note   唤醒时相当于一次“软重启”：时钟要重新配置（SystemInit）、
 *         SysTick / TIMER2 / RS485 都要重新初始化；
 *         睡眠期间秒计数停摆，醒来后补上 10 秒。
 * @note   注意：NVM 里的 RTC_WKUP_IRQHandler 必须挂在正确的中断向量上，
 *         否则 WFI 唤醒后会跳到默认处理函数而死循环。
 */
void App_Shui10s(uint32_t baudrate)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    rtc_wakeup_disable();
    rtc_wakeup_clock_set(WAKEUP_CKSPRE);            /* 1Hz 唤醒时钟 */
    rtc_wakeup_timer_set(9U);                       /* 9 + 1 = 10 个 1Hz 周期 */
    rtc_flag_clear(RTC_FLAG_WT);
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    exti_flag_clear(EXTI_22);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_WKUP_IRQn, 1U, 1U);
    rtc_wakeup_enable();

    /* 关掉会干扰唤醒的时钟源与中断，再进深度睡眠 */
    usart_interrupt_disable(USART1, USART_INT_RBNE);
    timer_disable(TIMER2);
    SysTick->CTRL = 0U;
    __DSB();
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_DISABLE, WFI_CMD);
    __ISB();

    /* 被 RTC 唤醒：收尾并恢复外设 */
    rtc_wakeup_disable();
    rtc_flag_clear(RTC_FLAG_WT);
    exti_flag_clear(EXTI_22);
    SystemInit();
    SystemCoreClockUpdate();
    systick_config();
    RS485_Config(baudrate);
    Tim_Init();
    s += 10U;                                       /* 补上睡眠期间停掉的秒数 */
}

/** @brief 读取 CH0（片内 ADC，PA0），ratio 为标定系数 */
float App_Du_CH0(float ratio)
{
    return Analog_ReadCH0() * ratio;
}

/** @brief 读取 CH1（DAC 回读电压），ratio 为标定系数 */
float App_Du_CH1(float ratio)
{
    return Analog_ReadCH1() * ratio;
}

/** @brief 读取 CH2（PT100 + AD3344，13.7488R + 97.6430 拟合） */
float App_Du_CH2(void)
{
    return pt100_read_temperature(0, 100.0f);
}

/** @brief 设置 DAC 输出码值（0~4095） */
void App_Set_Dac(uint16_t value)
{
    Analog_SetDac(value);
}
