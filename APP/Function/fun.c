#include "Head.h"

void Yingjian_Chushihua(void)
{
	// waishe chushihua
	spi_flash_init();
	Analog_Init();
	Xieyi_Chushihua();
	LED_Init();
	oled_init();
	pt100_ad3344_init(PT100_AD3344_SINGLE_END, 0, PT100_AD3344_DR_100SPS, PT100_AD3344_PGA_4_096V);
	rtc_config();
	Tim_Init();

	// shangdian xianshi zhuangtai
	oled_show_string(0, 0, "2026413756", 16);
	oled_show_string(0, 16, "IDLE", 16);
	oled_refresh_gram();
}

void Xieyi_Xunhuan_Renwu(void)
{
	// zhixu chuli RS485 xieyi
	Xieyi_Chuli();
}
