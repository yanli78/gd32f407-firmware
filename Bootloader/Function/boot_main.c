#include "Boot_Config.h"
#include "Boot_Flash.h"
#include "Boot_OLED.h"
#include "Boot_Protocol.h"
#include "Boot_RS485.h"
#include "Boot_Timer.h"

static uint8_t app_is_valid(void)
{
    uint32_t sp = *(volatile uint32_t *)BOOT_APP_ADDR;
    uint32_t pc = *(volatile uint32_t *)(BOOT_APP_ADDR + 4U);
    if((sp < 0x20000000U) || (sp > 0x20030000U)) return 0U;
    if((pc < BOOT_APP_ADDR) || (pc >= BOOT_APP_END) || ((pc & 1U) == 0U)) return 0U;
    return 1U;
}

static void jump_to_app(void)
{
    uint32_t sp = *(volatile uint32_t *)BOOT_APP_ADDR;
    uint32_t pc = *(volatile uint32_t *)(BOOT_APP_ADDR + 4U);
    uint32_t i;
    __disable_irq();
    SysTick->CTRL = 0U; SysTick->LOAD = 0U; SysTick->VAL = 0U;
    for(i = 0U; i < 8U; i++) { NVIC->ICER[i] = 0xFFFFFFFFU; NVIC->ICPR[i] = 0xFFFFFFFFU; }
    SCB->VTOR = BOOT_APP_ADDR;
    __DSB(); __ISB();
    __set_MSP(sp);
    __enable_irq();
    ((void (*)(void))pc)();
}

static void boot_wait_print(uint32_t left_s, uint8_t *last_print)
{
    if((left_s <= *last_print) && ((*last_print == 10U) || (*last_print == 7U) || (*last_print == 4U) || (*last_print == 1U))) {
        if(*last_print == 10U) Boot_RS485_SendString("wait for start Application(10s)......\r\n");
        else if(*last_print == 7U) Boot_RS485_SendString("wait for start Application(7s)......\r\n");
        else if(*last_print == 4U) Boot_RS485_SendString("wait for start Application(4s)......\r\n");
        else Boot_RS485_SendString("wait for start Application(1s)......\r\n");
        *last_print = (uint8_t)(*last_print - 3U);
    }
}

int main(void)
{
    uint32_t start;
    BootFlagTypeDef boot_flag;
    uint8_t upgrade_requested;
    uint16_t boot_device_id = BOOT_DEVICE_ID;
    uint32_t boot_baudrate = BOOT_BAUDRATE;
    SCB->VTOR = 0x08000000U;
    __DSB();
    __ISB();
    memcpy(&boot_flag, (const void *)BOOT_FLAG_ADDR, sizeof(boot_flag));
    upgrade_requested = (boot_flag.magic == BOOT_FLAG_MAGIC) ? 1U : 0U;
    if(upgrade_requested != 0U) {
        if((boot_flag.device_id != 0U) && (boot_flag.device_id != 0xFFFFU)) boot_device_id = boot_flag.device_id;
        if((boot_flag.baudrate == 4800U) || (boot_flag.baudrate == 9600U) || (boot_flag.baudrate == 19200U) || (boot_flag.baudrate == 115200U)) boot_baudrate = boot_flag.baudrate;
    }
    Boot_Timer_Init();
    Boot_RS485_Init(boot_baudrate);
    Boot_OLED_Init();
    Boot_Protocol_Init(boot_device_id);
    if(upgrade_requested != 0U) Boot_Flash_Erase(BOOT_FLAG_ADDR, 4096U);
    if(upgrade_requested == 0U) {
        start = Boot_S();
        while((Boot_S() - start) < 5U) {
            Boot_Protocol_Process();
            if(Boot_Protocol_Stay() != 0U) upgrade_requested = 1U;
        }
        if((upgrade_requested == 0U) && (app_is_valid() != 0U)) jump_to_app();
    } else {
        Boot_RS485_SendString("\r\nusing command to interrupt start Application\r\n");
    }
    start = Boot_S();
    {
        uint8_t last_print = 10U;
        while(1) {
            if(Boot_Protocol_Stay() == 0U) boot_wait_print(10U - (Boot_S() - start), &last_print);
            Boot_Protocol_Process();
            if((Boot_Protocol_Stay() == 0U) && (Boot_Protocol_UpgradeReady() == 0U) && ((Boot_S() - start) >= 10U) && (app_is_valid() != 0U)) jump_to_app();
        }
    }
}
