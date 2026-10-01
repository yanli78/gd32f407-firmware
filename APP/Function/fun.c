#include "Head.h"

/**
 * @brief  外设初始化（上电时调用一次）
 * @note   初始化顺序有依赖：
 *         RS485 协议层要读 SPI Flash 里的参数，所以 spi_flash_init() 必须在前；
 *         Xieyi_Chushihua() 里会按保存的参数调用 RS485_Config()；
 *         OLED 最后再显示开机画面。
 */
void Yingjian_Chushihua(void)
{
	/* 外设初始化 */
	spi_flash_init();
	Analog_Init();
	Xieyi_Chushihua();
	LED_Init();
	oled_init();
	pt100_ad3344_init(PT100_AD3344_SINGLE_END, 0, PT100_AD3344_DR_100SPS, PT100_AD3344_PGA_4_096V);
	rtc_config();
	Tim_Init();

	/* 上电显示状态 */
	oled_clear_gram();
	oled_show_string(0, 0, "2026413756", 16);
	oled_show_string(0, 16, "IDLE", 16);
	oled_refresh_gram();
}

/**
 * @brief  主循环任务：处理 RS485 协议（收帧、执行命令、自动上报）
 */
void Xieyi_Xunhuan_Renwu(void)
{
	/* 持续处理 RS485 协议 */
	Xieyi_Chuli();
}
