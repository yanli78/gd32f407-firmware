/************************************************************
 * 版权所有 2025CIMC Copyright
 * 文件名: main.c
 * 作者: Lingyu Meng
 * 平台: 2025 CIMC IHD V04
 * 版本: Lingyu Meng     2023/2/16     V0.01    original
************************************************************/

/************************* 头文件 *************************/

#include "HeaderFiles.h"



/************************ 执行函数 ************************/

int main(void)
{
	#ifdef APP_IMAGE
	SCB->VTOR = 0x08011000U;
	__DSB();
	__ISB();
	__enable_irq();
	#endif

	systick_config();
	
	Xitong_Chushihua();
	
	Yingyong_Renwu();
	
}


/****************************End*****************************/
