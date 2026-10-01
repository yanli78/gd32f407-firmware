/************************************************************
 * 版权所有 2025CIMC Copyright
 * 文件名: Function.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Head.h"

/************************* 宏定义 *************************/


/************************ 函数声明 ************************/


/************************ 函数定义 ************************/



/************************************************************ 
 * Function :       Xitong_Chushihua
 * Comment  :       用于初始化 MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void Xitong_Chushihua(void)
{
	// 先做硬件初始化
	Yingjian_Chushihua();
	
}
/************************************************************ 
 * Function :       Init_LED_Stat
 * Comment  :       系统初始化时的 LED 显示状态
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-10 V0.1 original
************************************************************/


/************************************************************ 
 * Function :       Yingyong_Renwu
 * Comment  :       用户主任务: LED1 闪烁
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void Yingyong_Renwu(void)
{
	delay_1ms(100U);
	Xieyi_Xintiao();

	// 主程序一直在这里运行
	while(1)
	{
		Xieyi_Xunhuan_Renwu();
	}
}


/****************************End*****************************/
