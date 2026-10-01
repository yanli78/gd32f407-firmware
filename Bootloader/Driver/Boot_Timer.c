#include "Boot_Timer.h"

static volatile uint32_t boot_ms = 0U;
static volatile uint32_t boot_s = 0U;

void Boot_Timer_Init(void)
{
    SysTick_Config(SystemCoreClock / 1000U);
}

void Boot_DelayMs(uint32_t ms)
{
    uint32_t start = boot_ms;
    while((boot_ms - start) < ms) {
    }
}

uint32_t Boot_Ms(void)
{
    return boot_ms;
}

uint32_t Boot_S(void)
{
    return boot_s;
}

void SysTick_Handler(void)
{
    boot_ms++;
    if((boot_ms % 1000U) == 0U) boot_s++;
}
