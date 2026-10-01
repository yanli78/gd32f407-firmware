/**
 ****************************************************************************************************
 * @file        oled.h
 * @version     V1.0
 * @brief       OLED 驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 正点原子
 *
 * 实验平台:    GD32F470VET6
 *
 ****************************************************************************************************
 */

#ifndef __OLED_H
#define __OLED_H

#include "Head.h"

/* 命令/数据 定义 */
#define OLED_CMD        0       /* 写命令 */
#define OLED_DATA       1       /* 写数据 */

/* SSD1306 从机地址（8 位写地址） */
#define OLED_IIC_ADDR   0x78

/******************************************************************************************/

void oled_init(void);                                                               /* OLED 初始化 */
void oled_clear(void);                                                              /* OLED 清屏：清显存并刷新屏幕 */
void oled_clear_gram(void);                                                         /* 只清显存，不刷新屏幕 */
void oled_display_on(void);                                                         /* 开启 OLED 显示 */
void oled_display_off(void);                                                        /* 关闭 OLED 显示 */
void oled_refresh_gram(void);                                                       /* 更新显存到 OLED */
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);                            /* OLED 画点 */
void oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot);        /* OLED 填充区域 */
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode); /* OLED 显示字符 */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);  /* OLED 显示数字 */
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size);           /* OLED 显示字符串 */
void OLED_Show_Font(uint16_t x, uint16_t y, uint8_t fnum);                          /* OLED 显示汉字 */

#endif
