/**
 ****************************************************************************************************
 * @file        myiic.c
 * @version     V1.0
 * @brief       OLED IIC 模式驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 正点原子
 *
 * 实验平台:    GD32F470VET6
 *
 ****************************************************************************************************
 */
 
#include "Head.h"
#include "sys.h"


/**
 * @brief       初始化 IIC
 * @param       无
 * @retval      无
 */
void iic_init(void)
{
    rcu_periph_clock_enable(OLED_IIC_SCL_GPIO_CLK);  /* SCL 引脚时钟使能 */
    rcu_periph_clock_enable(OLED_IIC_SDA_GPIO_CLK);  /* SDA 引脚时钟使能 */

    /* SCL 引脚模式设置，开漏输出，上拉 */
    gpio_mode_set(OLED_IIC_SCL_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_IIC_SCL_GPIO_PIN);
    gpio_output_options_set(OLED_IIC_SCL_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_IIC_SCL_GPIO_PIN);

    /* SDA 引脚模式设置，开漏输出，上拉，输出高电平时可释放总线，也可读取外部信号的高低电平 */
    gpio_mode_set(OLED_IIC_SDA_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_IIC_SDA_GPIO_PIN);
    gpio_output_options_set(OLED_IIC_SDA_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_IIC_SDA_GPIO_PIN);

    iic_stop();     /* 停止总线上所有设备 */
}

/**
 * @brief       IIC 延时函数，用于控制 IIC 读写速度
 * @param       无
 * @retval      无
 */
static void iic_delay(void)
{
    uint16_t t = 120U;
    while(t--) {
    }
}

/**
 * @brief       产生 IIC 起始信号
 * @param       无
 * @retval      无
 */
void iic_start(void)
{
    IIC_SDA(1);
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(0);     /* START 信号: 当 SCL 为高时，SDA 从高变低，表示起始信号 */
    iic_delay();
    IIC_SCL(0);     /* 钳住 I2C 总线，准备发送或接收数据 */
    iic_delay();
}

/**
 * @brief       产生 IIC 停止信号
 * @param       无
 * @retval      无
 */
void iic_stop(void)
{
    IIC_SDA(0);     /* STOP 信号: 当 SCL 为高时，SDA 从低变高，表示停止信号 */
    iic_delay();
    IIC_SCL(1);
    iic_delay();
    IIC_SDA(1);     /* 发送 I2C 总线结束信号 */
    iic_delay();
}

/**
 * @brief       等待应答信号到来
 * @param       无
 * @retval      1，接收应答失败
 *              0，接收应答成功
 */
uint8_t iic_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;

    IIC_SDA(1);     /* 主机释放 SDA 线(此时外部器件可以拉低 SDA 线) */
    iic_delay();
    IIC_SCL(1);     /* SCL=1，此时从机可以返回 ACK */
    iic_delay();

    gpio_mode_set(OLED_IIC_SDA_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, OLED_IIC_SDA_GPIO_PIN);
    while(IIC_READ_SDA) {
        waittime++;
        if(waittime > 250U) {
            iic_stop();
            rack = 1U;
            break;
        }
    }

    IIC_SCL(0);     /* SCL=0，结束 ACK 检测 */
    iic_delay();
    gpio_mode_set(OLED_IIC_SDA_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, OLED_IIC_SDA_GPIO_PIN);
    gpio_output_options_set(OLED_IIC_SDA_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, OLED_IIC_SDA_GPIO_PIN);
    return rack;
}

/**
 * @brief       产生 ACK 应答
 * @param       无
 * @retval      无
 */
void iic_ack(void)
{
    IIC_SDA(0);     /* SCL 0 -> 1 时 SDA = 0，表示应答 */
    iic_delay();
    IIC_SCL(1);     /* 产生一个时钟 */
    iic_delay();
    IIC_SCL(0);
    iic_delay();
    IIC_SDA(1);     /* 主机释放 SDA 线 */
    iic_delay();
}

/**
 * @brief       不产生 ACK 应答
 * @param       无
 * @retval      无
 */
void iic_nack(void)
{
    IIC_SDA(1);     /* SCL 0 -> 1 时 SDA = 1，表示不应答 */
    iic_delay();
    IIC_SCL(1);     /* 产生一个时钟 */
    iic_delay();
    IIC_SCL(0);
    iic_delay();
}

/**
 * @brief       IIC 发送一个字节
 * @param       data: 要发送的数据
 * @retval      无
 */
void iic_send_byte(uint8_t data)
{
    uint8_t t;
    
    for (t = 0; t < 8; t++)
    {
        IIC_SDA((data & 0x80) >> 7);    /* 高位先发送 */
        iic_delay();
        IIC_SCL(1);
        iic_delay();
        IIC_SCL(0);
        data <<= 1;     /* 左移 1 位，准备下一次发送 */
    }
    
    IIC_SDA(1);         /* 发送完成，主机释放 SDA 线 */
}

/**
 * @brief       IIC 读取一个字节
 * @param       ack:  ack=1 时发送 ack；ack=0 时发送 nack
 * @retval      接收到的数据
 */
uint8_t iic_read_byte(uint8_t ack)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++ )    /* 接收 1 个字节数据 */
    {
        receive <<= 1;          /* 高位先输出，所以接收到的数据位要左移 */
        IIC_SCL(1);
        iic_delay();

        if (IIC_READ_SDA)
        {
            receive++;
        }
        
        IIC_SCL(0);
        iic_delay();
    }

    if (!ack)
    {
        iic_nack();             /* 发送 nACK */
    }
    else
    {
        iic_ack();              /* 发送 ACK */
    }

    return receive;             /* 返回读到的数据 */
}


static uint32_t g_fac_us = 0;      /* us 延时倍乘数 */


void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload;
    if (g_fac_us == 0) {
        g_fac_us = SystemCoreClock / 1000000U;
    }
    reload = SysTick->LOAD;             /* LOAD 的值 */
    ticks = nus * g_fac_us;             /* 需要的节拍数 */
  
#if SYS_SUPPORT_OS                      /* 如果需要支持 OS */
    delay_osschedlock();                /* 锁定 OS 调度器，防止打断 us 延时 */
#endif
  
    told = SysTick->VAL;                /* 刚进入时的计数器值 */
  
    while (1)
    {
        tnow = SysTick->VAL;

        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;    /* 递减计数器，可直接相减 */
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            
            told = tnow;
            
            if (tcnt >= ticks) 
            {
                break;                  /* 时间超过/等于需要延时的时间，退出 */
            }
        }
    }
    
}






















