#include "Tim.h"

uint32_t s;




//void Tim_Init(void)
//{
//	timer_parameter_struct timer_initpara;

//	rcu_periph_clock_enable(RCU_TIMER2);  // 使能定时器时钟

//	timer_deinit(TIMER2);  // 复位 TIMER2 配置

//	timer_initpara.prescaler         = 71999;  // 分频后 240MHz 到 10kHz
//	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
//	timer_initpara.counterdirection  = TIMER_COUNTER_UP;
//	timer_initpara.period            = 9999;   // 每 1 秒触发
//	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
//	timer_initpara.repetitioncounter = 0;

//	timer_init(TIMER2, &timer_initpara);

//	// 中断配置
//	nvic_irq_enable(TIMER2_IRQn, 1, 0);
//	timer_interrupt_enable(TIMER2, TIMER_INT_UP);
//	timer_enable(TIMER2);
//	
//	
//}



//void TIMER2_IRQHandler(void)
//{
//    if (timer_interrupt_flag_get(TIMER2, TIMER_INT_UP)) {
//        timer_interrupt_flag_clear(TIMER2, TIMER_INT_UP);
//		s++;
//        // gpio_bit_toggle(GPIOC, GPIO_PIN_13);
//    }
//}


void Tim_Init(void)
{
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER2);  // 使能定时器时钟

    timer_deinit(TIMER2);  // 复位 TIMER2 配置

    timer_initpara.prescaler         = 23999;  // 分频后 72MHz 到 1kHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 4999;   // 每 1 秒触发
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    timer_init(TIMER2, &timer_initpara);

    // 中断配置
    nvic_irq_enable(TIMER2_IRQn, 0, 0); // 设置优先级
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);
    timer_enable(TIMER2);
}

// 定时器中断处理
void TIMER2_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER2, TIMER_INT_UP))
    {
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_UP);
        s++;
    }
}
