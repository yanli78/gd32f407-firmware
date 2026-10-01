#include "Head.h"



void LED_Init(void)
{
	
	
	
	rcu_periph_clock_enable(LED1_Port1);
	
	gpio_mode_set(LED1_Port,GPIO_MODE_OUTPUT,GPIO_PUPD_NONE,LED1_Pin);
	gpio_output_options_set(LED1_Port,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,LED1_Pin);
	gpio_bit_reset(LED1_Port,LED1_Pin);											// 引脚初始电平为低电平
	
	
	gpio_mode_set(LED2_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED2_Pin);   			// GPIO 模式设置为输出
    gpio_output_options_set(LED2_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED2_Pin);   // 推挽输出设置
	gpio_bit_reset(LED2_Port, LED2_Pin);
	
//	gpio_mode_set(LED1_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED1_Pin);   			// GPIO 模式设置为输出
//    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED1_Pin);   // 推挽输出设置
//	gpio_bit_reset(LED1_Port, LED1_Pin);
}



