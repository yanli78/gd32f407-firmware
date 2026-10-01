/**
 ****************************************************************************************************
 * @file        oled.c
 * @version     V1.0
 * @brief       OLED 驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 正点原子
 *
 * 实验平台:    GD32F470VET6
 *
 ****************************************************************************************************
 */
 

#include "oledfont.h"
#include "Head.h"

/* 
 * OLED 显存
 * 每个字节表示 8 个像素，128 表示有 128 列，4 表示有 32 行，低位表示在前面。
 * 例如: g_oled_gram[0][0]，包含第一列第 1~8 行的数据。g_oled_gram[0][0].0，表示坐标(0,0)
 * 类推: g_oled_gram[1][0].1，表示坐标(1,1)，g_oled_gram[10][1].2，表示坐标(10,10)，
 * 
 * 存放格式如下(低位表示在前面)。
 * [0]0 1 2 3 ... 127
 * [1]0 1 2 3 ... 127
 * [2]0 1 2 3 ... 127
 * [3]0 1 2 3 ... 127
 */
static uint8_t g_oled_gram[128][4];

/* 本文件内部使用的写字节接口（原声明放在 OLED.h 里，static 函数不应出现在头文件） */
static void oled_wr_byte(uint8_t data, uint8_t cmd);

/**
 * @brief       设置 OLED 显示位置：页地址 + 列地址
 * @note        3 个命令字节合并到同一次 IIC 传输（控制字节 0x00），
 *              比逐字节发送少 2 次 START/STOP，一次整屏刷新省 8 次。
 * @param       page: 页地址 0~3
 * @retval      无
 */
static void oled_set_page(uint8_t page)
{
    iic_start();
    iic_send_byte(OLED_IIC_ADDR);
    iic_wait_ack();
    iic_send_byte(0x00);                /* 控制字节：后续均为命令 */
    iic_wait_ack();
    iic_send_byte(0xB0 + page);         /* 设置页地址（0~3） */
    iic_wait_ack();
    iic_send_byte(0x00);                /* 设置显示位置：列低地址 */
    iic_wait_ack();
    iic_send_byte(0x10);                /* 设置显示位置：列高地址 */
    iic_wait_ack();
    iic_stop();
}

/**
 * @brief       更新显存到 OLED
 * @param       无
 * @retval      无
 */
void oled_refresh_gram(void)
{
    uint8_t i, n;

    for (i = 0; i < 4; i++)
    {
        oled_set_page(i);

        iic_start();
        iic_send_byte(OLED_IIC_ADDR);
        iic_wait_ack();
        iic_send_byte(0x40);            /* 控制字节：后续均为数据 */
        iic_wait_ack();

        for (n = 0; n < 128; n++)
        {
            iic_send_byte(g_oled_gram[n][i]);
            iic_wait_ack();
        }
        iic_stop();
    }
}

/**
 * @brief       向 OLED 写入一个字节
 * @param       data: 要写入的命令/数据
 * @param       cmd: 命令/数据标志 0，表示命令；1，表示数据；
 * @retval      无
 */
static void oled_wr_byte(uint8_t data, uint8_t cmd)
{
    iic_start();                        /* 发送起始信号 */
    iic_send_byte(OLED_IIC_ADDR);       /* 发送从机地址 */
    iic_wait_ack();                     /* 每次发送完一个字节，都要等待 ACK 时钟信号 */

    if(cmd == OLED_CMD)
    {
        iic_send_byte(0x00);            /* 发送写命令时的控制字节 */
    }
    else
    {
        iic_send_byte(0x40);            /* 发送写数据时的控制字节 */
    }

    iic_wait_ack();                     /* 每次发送完一个字节，都要等待 ACK 时钟信号 */
    iic_send_byte(data);                /* 发送数据字节 */
    iic_wait_ack();                     /* 等待 ACK 时钟信号 */
    iic_stop();                         /* 发送停止信号 */
}

/**
 * @brief       开启 OLED 显示
 * @param       无
 * @retval      无
 */
void oled_display_on(void)
{
    oled_wr_byte(0X8D, OLED_CMD);   /* SET DCDC 命令 */
    oled_wr_byte(0X14, OLED_CMD);   /* DCDC ON */
    oled_wr_byte(0XAF, OLED_CMD);   /* DISPLAY ON */
}

/**
 * @brief       关闭 OLED 显示
 * @param       无
 * @retval      无
 */
void oled_display_off(void)
{
    oled_wr_byte(0X8D, OLED_CMD);   /* SET DCDC 命令 */
    oled_wr_byte(0X10, OLED_CMD);   /* DCDC OFF */
    oled_wr_byte(0XAE, OLED_CMD);   /* DISPLAY OFF */
}

/**
 * @brief       只清显存，不刷新屏幕
 * @note        显存清空后屏幕仍是旧画面，需要调用者自己调用 oled_refresh_gram()。
 *              一次整屏 IIC 刷新约 20ms，连续「清屏 + 写内容」时用它可省一次刷新。
 * @param       无
 * @retval      无
 */
void oled_clear_gram(void)
{
    memset(g_oled_gram, 0, sizeof(g_oled_gram));
}

/**
 * @brief       清屏函数，清完屏后整个屏幕为黑色
 * @param       无
 * @retval      无
 */
void oled_clear(void)
{
    oled_clear_gram();
    oled_refresh_gram();    /* 更新显示 */
}

/**
 * @brief       OLED 画点
 * @param       x  : 0~127
 * @param       y  : 0~31（屏幕为 128x32，显存 4 页）
 * @param       dot: 1 画点 0，清除
 * @retval      无
 */ 
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot)
{
    uint8_t pos, bx, temp = 0;

    if (x > 127 || y > 31)
    {
        return;                     /* 超出范围 */
    }

    pos = y / 8;                    /* 计算 GRAM 中 y 坐标所在字节，每个字节可以存储 8 个像素 */

    bx = y % 8;                     /* 取余数，计算 y 在对应字节里的位位置 */
    temp = (uint8_t)(1 << bx);      /* 得到 y 对应的 bit 位置，将该 bit 置 1 */

    if (dot)                        /* 画实心点 */
    {
        g_oled_gram[x][pos] |= temp;
    }
    else                            /* 画空点，不显示 */
    {
        g_oled_gram[x][pos] &= (uint8_t)~temp;
    }
}

/**
 * @brief       OLED 填充区域
 * @note:       注意: 必须确保 x1<=x2; y1<=y2  0<=x1<=127  0<=y1<=31
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标
 * @param       dot: 1 画点 0，清除
 * @retval      无
 */ 
void oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot)
{
    uint8_t x, y;

    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
        {
            oled_draw_point(x, y, dot);
        }
    }

    oled_refresh_gram();    /* 更新显示 */
}

/**
 * @brief       在指定位置显示一个字符，包含部分字符
 * @param       x   : 0~127
 * @param       y   : 0~31
 * @param       size: 选择字体 12/16/24
 * @param       mode: 0，反白显示；1，正常显示
 * @retval      无
 */ 
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode)
{
    uint8_t temp, t, t1;
    uint8_t y0 = y;
    const uint8_t *pfont = 0;
    uint8_t csize = (uint8_t)((size / 8 + ((size % 8) ? 1 : 0)) * (size / 2)); /* 得到字体一个字符对应点阵集所占的字节数 */

    if ((chr < ' ') || (chr > '~')) return;                         /* 字库只包含可显示 ASCII */
    chr = (uint8_t)(chr - ' ');                                     /* 得到偏移后的值，因为字库从空格开始存储，第一个字符是空格 */

    if (y + size > 32)    return;                                   /* 高度超限 */
	
    if (size == 12)                                                 /* 调用 1206 字体 */
    {
        pfont = (const uint8_t *)oled_asc2_1206[chr];        
    }
    else if (size == 16)                                            /* 调用 1608 字体 */ 
    {
        pfont = (const uint8_t *)oled_asc2_1608[chr];
    }
    else if (size == 24)                                            /* 调用 2412 字体 */
    { 
        pfont = (const uint8_t *)oled_asc2_2412[chr];
    }
    else                                                            /* 没有的字库 */
    {
        return;   
    }
    
    for (t = 0; t < csize; t++)
    { 
        temp = pfont[t];
      
        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
            {
                oled_draw_point(x, y, mode);
            }
            else
            {
                oled_draw_point(x, y, (uint8_t)!mode);
            }

            temp <<= 1;
            y++;

            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @brief       平方函数，m^n
 * @param       m: 底数
 * @param       n: 指数
 * @retval      结果
 */
static uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)
    {
        result *= m;
    }

    return result;
}

/**
 * @brief       显示 len 个数字
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24
 * @retval      无
 */  
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;

    for (t = 0; t < len; t++)                                           /* 按照显示位数循环 */
    {
        temp = (uint8_t)((num / oled_pow(10, (uint8_t)(len - t - 1))) % 10); /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))                               /* 还没有使能显示，且还有位要显示 */
        {
            if (temp == 0)
            {
                oled_show_char((uint8_t)(x + (size / 2) * t), y, ' ', size, 1); /* 显示空格，占位 */
                continue;                                               /* 继续下一个位 */
            }
            else
            {
                enshow = 1;                                             /* 使能显示 */
            }
        }

        oled_show_char((uint8_t)(x + (size / 2) * t), y, (uint8_t)(temp + '0'), size, 1); /* 显示字符 */
    }
} 

/**
 * @brief       显示字符串
 * @param       x,y : 起始坐标
 * @param       size: 选择字体 12/16/24
 * @param       *p  : 字符串指针，指向字符串首地址
 * @retval      无
 */ 
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size)
{
    while ((*p <= '~') && (*p >= ' '))      /* 判断是否为非法字符 */
    {
        if (x > (128 - (size / 2)))         /* 宽度越界 */
        {
            x = 0;
            y += size;                      /* 换行 */
        }

        if (y > (64 - size))                /* 高度越界 */
        {
            y = x = 0;
            oled_clear_gram();              /* 只清显存，本次内容统一在刷新时写出 */
        }

        oled_show_char(x, y, (uint8_t)*p, size, 1);  /* 显示一个字符 */
        x += size / 2;                      /* ASCII 字符宽度为汉字宽度的一半 */
        p++;
    }
}
 
/**
 * @brief       在指定位置显示一个 24*24 大小的汉字
 * @param       x,y : 起始坐标
 * @param       fnum: 汉字编号，字库数组中的编号
 * @retval      无
 */ 
void OLED_Show_Font(uint16_t x, uint16_t y, uint8_t fnum)
{
	if (y + 24 > 32)   return;  /* 限制汉字显示在屏幕范围内 */
        
	uint8_t temp, t, t1;
	uint16_t y0 = y;
	const uint8_t *dzk;   
	uint8_t csize = 72;					/* 一个 24*24 的汉字 72 字节 */
	
	dzk = (const uint8_t *)OLED_HZK_XMZB[fnum];	/* 得到汉字编号对应的点阵码 */
	
	for(t = 0; t < csize; t++)
	{   												   
		temp = dzk[t];				/* 得到点阵数据 */                          
		for(t1 = 0; t1 < 8; t1++)
		{
			if(temp & 0x80) oled_draw_point((uint8_t)x, (uint8_t)y, 1);
			else oled_draw_point((uint8_t)x, (uint8_t)y, 0); 
			temp <<= 1;
			y++;
			if((y - y0) == 24)
			{
				y = y0;
				x++;
				break;
			}
		}  	 
	}  
}

/**
 * @brief       初始化 OLED(SSD1306)
 * @param       无
 * @retval      无
 */
void oled_init(void)
{ 
	iic_init();     /* IIC 接口初始化 */
	delay_1ms(500U);

    oled_wr_byte(0xAE, OLED_CMD);   /* 关闭显示 */
    oled_wr_byte(0x00, OLED_CMD);
    oled_wr_byte(0x10, OLED_CMD);
    oled_wr_byte(0x40, OLED_CMD);
    oled_wr_byte(0x81, OLED_CMD);
    oled_wr_byte(0xCF, OLED_CMD);
    oled_wr_byte(0xA1, OLED_CMD);
    oled_wr_byte(0xC8, OLED_CMD);
    oled_wr_byte(0xA6, OLED_CMD);
    oled_wr_byte(0xA8, OLED_CMD);
    oled_wr_byte(0x1F, OLED_CMD);
    oled_wr_byte(0xD3, OLED_CMD);
    oled_wr_byte(0x00, OLED_CMD);
    oled_wr_byte(0xD5, OLED_CMD);   /* 设置时钟分频因子，振荡频率 */
    oled_wr_byte(0x80, OLED_CMD);   /* [3:0]，分频因子；[7:4]，振荡频率 */
    oled_wr_byte(0xD9, OLED_CMD);
    oled_wr_byte(0xF1, OLED_CMD);
    oled_wr_byte(0xDA, OLED_CMD);
    oled_wr_byte(0x00, OLED_CMD);
    oled_wr_byte(0xDB, OLED_CMD);
    oled_wr_byte(0x40, OLED_CMD);
    oled_wr_byte(0x20, OLED_CMD);
    oled_wr_byte(0x02, OLED_CMD);
    oled_wr_byte(0x8D, OLED_CMD);
    oled_wr_byte(0x14, OLED_CMD);
    oled_wr_byte(0xA4, OLED_CMD);
    oled_wr_byte(0xA6, OLED_CMD);
    oled_wr_byte(0xAF, OLED_CMD);   /* 开启显示 */
    oled_clear();                   /* 上电清屏（面板刚上电，需真正刷新一次） */
}
