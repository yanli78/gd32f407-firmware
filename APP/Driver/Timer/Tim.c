#include "Tim.h"

/* 系统秒计数：TIMER2 更新中断里写、主循环与协议层里读。
   跨中断共享，必须 volatile，否则编译器可能把它缓存到寄存器里。 */
volatile uint32_t s = 0U;

/* ---------------------------------------------------------------------------
 * TIMER2 时基（1Hz 更新中断，即 s 每秒 +1）
 *
 * GD32F470VE 主频 240MHz（system_gd32f4xx.c 选中 __SYSTEM_CLOCK_240M_PLL_25M_HXTAL）：
 *   HCLK = 240MHz，APB1 = 60MHz，APB1 上的定时器时钟 = 2 x APB1 = 120MHz
 *   预分频到 5kHz（24000 分频）后，再计 5000 个数 -> 1Hz
 *   即 prescaler = 23999、period = 4999（与原工程写死的值一致）
 *
 * 这里由 SystemCoreClock 推导寄存器值：240MHz 时结果与原工程完全相同，
 * 以后若改了主频也仍然能得到 1 秒节拍，而不是静默跑偏。
 * ------------------------------------------------------------------------- */
#define TIM_TIMER_CLK_DIV   2U       /* 定时器时钟 = SystemCoreClock / 2 */
#define TIM_TICK_HZ         5000U    /* 预分频后的计数频率               */
#define TIM_UPDATE_HZ       1U       /* 期望的更新中断频率               */
#define TIM_PSC_MAX         0xFFFFU  /* prescaler / period 均为 16 位    */

void Tim_Init(void)
{
    timer_parameter_struct timer_initpara;
    uint32_t timer_clk = SystemCoreClock / TIM_TIMER_CLK_DIV;
    uint32_t prescaler = (timer_clk / TIM_TICK_HZ) - 1U;
    uint32_t period = (TIM_TICK_HZ / TIM_UPDATE_HZ) - 1U;

    /* 时钟异常（过快/过慢）时退回原工程固定值，保证秒中断始终存在 */
    if((timer_clk < TIM_TICK_HZ) || (prescaler > TIM_PSC_MAX) || (period > TIM_PSC_MAX)) {
        prescaler = 23999U;
        period = 4999U;
    }

    rcu_periph_clock_enable(RCU_TIMER2);    /* 使能定时器时钟 */
    timer_deinit(TIMER2);                   /* 复位 TIMER2 配置 */

    timer_initpara.prescaler = (uint16_t)prescaler;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (uint16_t)period;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0U;
    timer_init(TIMER2, &timer_initpara);

    nvic_irq_enable(TIMER2_IRQn, 0U, 0U);
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);
    timer_enable(TIMER2);
}

/* 定时器中断处理：每秒进入一次，递增系统秒计数 */
void TIMER2_IRQHandler(void)
{
    if(timer_interrupt_flag_get(TIMER2, TIMER_INT_UP) != RESET) {
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_UP);
        s++;
    }
}
