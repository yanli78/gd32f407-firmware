/************************************************************
 * 版权所有 2025CIMC Copyright
 * 文件名: main.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/

/************************* 头文件 *************************/

#include "HeaderFiles.h"


/************************ 执行函数 ************************/

int main(void)
{
	#ifdef APP_IMAGE
	/* 本工程作为带 Bootloader 的 APP 运行：中断向量表在 0x08011000，
	   必须在使能中断之前把 VTOR 指过去（Bootloader 跳转前也已设置，这里再确认一次）。 */
	SCB->VTOR = 0x08011000U;
	__DSB();
	__ISB();
	__enable_irq();
	#endif

	systick_config();       /* 1ms SysTick，供 delay_1ms 使用 */

	Xitong_Chushihua();     /* 初始化外设 */

	Yingyong_Renwu();       /* 主任务，内部为 while(1)，不会返回 */
}


/****************************End*****************************/
