/**
 ****************************************************************************************************					 
 * @file        sys.c
 * @version     V1.0
 * @brief       系统相关代码
 ****************************************************************************************************
 *
 * V1.0
 * 将头文件包含路径改为相对路径，避免重复配置包含路径的麻烦
 *
 ****************************************************************************************************
 */ 
 
#include "Head.h"


/**
 * @brief       执行 WFI 指令(执行完该指令进入低功耗状态，等待中断唤醒)
 * @param       无
 * @retval      无
 */
void sys_wfi_set(void)
{
    __ASM volatile("wfi");
}

/**
 * @brief       关闭所有中断(但是不包括 fault 和 NMI 中断)
 * @param       无
 * @retval      无
 */
void sys_intx_disable(void)
{
    __ASM volatile("cpsid i");
}

/**
 * @brief       开启所有中断
 * @param       无
 * @retval      无
 */
void sys_intx_enable(void)
{
    __ASM volatile("cpsie i");
}

/**
 * @brief       设置栈顶地址
 * @param       addr: 栈顶地址
 * @retval      无
 */
void sys_msr_msp(uint32_t addr)
{
    __set_MSP(addr);        /* 设置栈顶地址 */
}

/**
 * @brief       系统软复位
 * @param       无
 * @retval      无
 */
void sys_soft_reset(void)
{
    NVIC_SystemReset();
}






