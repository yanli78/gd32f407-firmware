/**
 ****************************************************************************************************
 * @file        rtc.c
 * @version     V1.0
 * @brief       RTC 驱动代码
 ****************************************************************************************************
 * @attention   Waiken-Smart 正点原子
 *
 * 实验平台:    GD32F470VET6
 *
 ****************************************************************************************************
 */
 
#include "Head.h"
//#include "gd32f4xx.h"
//#include "gd32f4xx_rtc.h"

//#define RTC_CLOCK_SOURCE_LXTAL  // 或 RTC_CLOCK_SOURCE_IRC32K


//uint32_t RTCSRC_FLAG = 0;
rtc_parameter_struct rtc_initpara;  /* RTC 参数结构体 */
    
/**
 * @brief       十进制转换为 BCD 码
 * @param       val : 要转换的十进制数
 * @retval      BCD 码
 */
uint8_t rtc_dec2bcd(uint8_t val)
{
    uint8_t bcdhigh = 0;

    while (val >= 10)
    {
        bcdhigh++;
        val -= 10;
    }

    return ((uint8_t)(bcdhigh << 4) | val);
}

/**
 * @brief       BCD 码转换为十进制数据
 * @param       val : 要转换的 BCD 码
 * @retval      十进制数据
 */
static uint8_t rtc_bcd2dec(uint8_t val)
{
    uint8_t temp = 0;
    temp = (val >> 4) * 10;
    return (temp + (val & 0X0F));
}

/**
 * @brief       RTC 时间设置
 * @param       hour,min,sec: 小时，分钟，秒钟
 * @param       ampm        : AM/PM, 0=AM/24H; 1=PM/12H;
 * @retval      0，成功
 *              1，初始化失败
 */
uint8_t rtc_set_time(uint8_t hour, uint8_t min, uint8_t sec, uint8_t ampm)
{
    ErrStatus error_status = ERROR;   
  
    rtc_initpara.hour   = rtc_dec2bcd(hour);       
    rtc_initpara.minute = rtc_dec2bcd(min);       
    rtc_initpara.second = rtc_dec2bcd(sec);      
    rtc_initpara.am_pm  = ((uint32_t)ampm & 0X01);
    
    error_status = rtc_init(&rtc_initpara);   /* 根据参数初始化 RTC 寄存器 */
    
    if (ERROR == error_status) return 1;
    
    return 0;  
}

/**
 * @brief       RTC 日期设置
 * @param       year,month,date : 年(0~99)，月(1~12)，日(1~31)
 * @param       week            : 星期(1~7，星期一~星期日；0，非法)
 * @retval      0，成功
 *              1，初始化失败
 */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week)
{
   ErrStatus error_status = ERROR;   
  
    rtc_initpara.year        = rtc_dec2bcd(year);       
    rtc_initpara.month       = rtc_dec2bcd(month);       
    rtc_initpara.date        = rtc_dec2bcd(date);      
    rtc_initpara.day_of_week = (week & 0X07);
    
    error_status = rtc_init(&rtc_initpara);   /* 根据参数初始化 RTC 寄存器 */
    
    if (ERROR == error_status) return 1;
    
    return 0;  
}

/**
 * @brief       获取 RTC 时间
 * @param       *hour,*min,*sec : 小时，分钟，秒钟
 * @param       *ampm           : AM/PM,0=AM/24H,1=PM/12H.
 * @retval      无
 */
void rtc_get_time(uint8_t *hour, uint8_t *min, uint8_t *sec, uint8_t *ampm)
{
    rtc_current_time_get(&rtc_initpara);   /* 获取当前日期和时间 */
  
    *hour = rtc_bcd2dec(rtc_initpara.hour);
    *min  = rtc_bcd2dec(rtc_initpara.minute);
    *sec  = rtc_bcd2dec(rtc_initpara.second);
    *ampm = (uint8_t)(rtc_initpara.am_pm >> 22);
}

/**
 * @brief       获取 RTC 日期
 * @param       *year,*mon,*date: 年，月，日
 * @param       *week           : 星期
 * @retval      无
 */
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week)
{
    rtc_current_time_get(&rtc_initpara);   /* 获取当前日期和时间 */  
  
    *year  = rtc_bcd2dec(rtc_initpara.year);
    *month = rtc_bcd2dec(rtc_initpara.month);
    *date  = rtc_bcd2dec(rtc_initpara.date);
    *week  = rtc_initpara.day_of_week;
}

/* 月份数据表 */
uint8_t const table_week[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};

/**
 * @brief       根据年月日计算星期，输入公历日期得到星期(只允许 1901-2099 年)
 * @param       year,month,day：公历年月日
 * @retval      星期数(1~7，星期一~星期日)
 */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t day)
{
    uint16_t temp;
    uint8_t yearh, yearl;

    yearh = year / 100;
    yearl = year % 100;

    /* 如果为 21 世纪，年份加 100 */
    if (yearh > 19)
    {
        yearl += 100;
    }

    /* 所过闰年数只算 1900 年之后的 */
    temp = yearl + yearl / 4;
    temp = temp % 7;
    temp = temp + day + table_week[month - 1];

    if (yearl % 4 == 0 && month < 3)
    {
        temp--;
    }

    temp %= 7;

    if (temp == 0)
    {
        temp = 7;
    }

    return temp;
}

/**
 * @brief       RTC 初始化
 * @note 
 *              默认尝试使用 LSE，当 LSE 启动失败后，切换为 LSI。
 *              通过 BKP 寄存器 0 的值，可以判断 RTC 使用的是 LSE/LSI:
 *              当 BKP0==0X7050 时，使用的是 LSE
 *              当 BKP0==0X7051 时，使用的是 LSI
 *              注意: 切换 LSI/LSE 会导致时间/日期丢失，切换后需要重新设置。
 *
 * @param       无
 * @retval      0，成功
 *              1，开启时钟失败
 */
uint8_t rtc_config(void)
{
    uint16_t prescaler_a = 0;                           /* RTC 异步分频值 */
    uint16_t prescaler_s  = 0;                          /* RTC 同步分频值 */
    uint8_t RTCSRC_FLAG = 0;
    uint16_t bkpflag = 0;
  
    rcu_periph_clock_enable(RCU_PMU);                   /* 使能电源接口时钟 */
    pmu_backup_write_enable();                          /* 备份域写使能 */   
    bkpflag = RTC_BKP0;                                 /* 读取 BKP0 的值 */
    rcu_osci_on(RCU_LXTAL);                             /* 开启外部低速晶振 */
		
    if ((rcu_osci_stab_wait(RCU_LXTAL) == ERROR))       /* 开启 CK_LXTAL 失败? */
    { 
        rcu_osci_on(RCU_IRC32K);                        /* 开启 CK_IRC32K 时钟 */     
      
        if ((rcu_osci_stab_wait(RCU_IRC32K) == ERROR))  /* 开启 CK_IRC32K 失败? */
        {
            return 1;                                   /* 开启 RTC 时钟源失败 */
        }
        
        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);        /* 选择 CK_IRC32K 时钟作为 RTC 的时钟源 */
        prescaler_s = 0x13F;                            /* RTC 同步分频值(0~7FFF) */
        prescaler_a = 0x63;                             /* RTC 异步分频值(0~0X7F) */
        RTC_BKP0 = 0X7051;                              /* 标记 RTC 使用 LSI */
    }	
    else
    {  
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);         /* 选择 RCU_LXTAL 时钟作为 RTC 的时钟源 */
        prescaler_s = 0xFF;
        prescaler_a = 0x7F;
        RTC_BKP0 = 0X7050;                              /* 标记 RTC 使用 LSE */
    }
    
    rcu_periph_clock_enable(RCU_RTC);                   /* 使能 RTC 时钟 */
    rtc_register_sync_wait();                           /* 等待同步 */                  
    RTCSRC_FLAG = GET_BITS(RCU_BDCTL, 8, 9);            /* 获取 RTC 时钟源选择 */ 
    
    /* BKP0 的数据既不是 0X7050，也不是 0X7051，说明是第一次配置，或者 RTC 时钟源没有配置，需要配置时钟和时间。 */
    if(((bkpflag != 0x7050) && (bkpflag != 0x7051)) || (0x00 == RTCSRC_FLAG))    
    {  
        rtc_deinit();
		rtc_initpara.factor_asyn = prescaler_a;         /* 设置 RTC 异步分频系数 */
        rtc_initpara.factor_syn = prescaler_s;          /* 设置 RTC 同步分频系数 */
        rtc_initpara.display_format = RTC_24HOUR;       /* RTC 时间格式为 24 小时格式 */
        rtc_set_time(23, 59, 50, 0);                    /* 设置时间 */
        rtc_set_date(24, 11, 13, 3);                    /* 设置日期 */
    
    }  
    
    return 0;    
}





/**
 * @brief       设置闹钟时间(按星期闹钟，24 小时制)
 * @param       week        : 星期几(1~7)
 * @param       hour,min,sec: 小时，分钟，秒钟
 * @retval      无
 */
void rtc_set_alarma(uint8_t week, uint8_t hour, uint8_t min, uint8_t sec)
{
    rtc_alarm_struct  rtc_alarm; 
  
    rtc_alarm_disable(RTC_ALARM0);                           /* 关闭 RTC 闹钟 0 */
    rtc_alarm.alarm_mask = 0;
    rtc_alarm.weekday_or_date = RTC_ALARM_WEEKDAY_SELECTED;  /* 按星期闹钟 */
    rtc_alarm.alarm_day    = rtc_dec2bcd(week) ;             /* 星期设置 */
    rtc_alarm.alarm_hour   = rtc_dec2bcd(hour) ;             /* 小时设置 */
    rtc_alarm.alarm_minute = rtc_dec2bcd(min) ;              /* 分钟设置 */
    rtc_alarm.alarm_second = rtc_dec2bcd(sec);               /* 秒钟设置 */
    rtc_alarm.am_pm = RTC_AM;                                /* 24 小时制 */  
    rtc_alarm_config(RTC_ALARM0, &rtc_alarm);                /* 配置 RTC 闹钟 0 */ 
  
    rtc_interrupt_enable(RTC_INT_ALARM0);                    /* 使能闹钟 0 中断 */
    rtc_alarm_enable(RTC_ALARM0);                            /* 使能 RTC 闹钟 0 */  
    rtc_flag_clear(RTC_FLAG_ALRM0);                          /* 清除 RTC 闹钟 0 触发标志 */   
  
    exti_flag_clear(EXTI_17);                                /* 清除 Line17 上的中断标志位 */
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);    /* 设置 Line17 上的中断，配置为上升沿触发中断 */
    nvic_irq_enable(RTC_Alarm_IRQn, 1, 2);                   /* 设置优先级，抢占优先级为 1，响应优先级为 2 */
}

/**
 * @brief       自动唤醒定时器设置
 * @param       wksel
 *   @arg       000,RTC/16;001,RTC/8;010,RTC/4;011,RTC/2;
 *   @arg       10x,ck_spre,1Hz;11x,1Hz，加 cnt 值再加 2^16(即 cnt+2^16)
 * @note        注意: RTC 这里表示 RTC 的时钟频率，不是 RTCCLK!
 * @param       cnt: 自动重装载值。减到 0，产生中断。
 * @retval      无
 */
void rtc_set_wakeup(uint8_t wksel, uint16_t cnt)
{    
    rtc_wakeup_disable();                  /* 失能 RTC 自动唤醒 */
   
    rtc_wakeup_clock_set(wksel);           /* 设置 RTC 自动唤醒定时器时钟 */
    
    rtc_wakeup_timer_set(cnt);             /* 设置自动唤醒定时器值 */
    
    rtc_flag_clear(RTC_FLAG_WT);           /* 清除自动唤醒触发标志 */
   
    rtc_interrupt_enable(RTC_INT_WAKEUP);  /* 使能 RTC 自动唤醒中断 */
  
    rtc_wakeup_enable();                   /* 使能 RTC 自动唤醒功能 */
  
    exti_flag_clear(EXTI_22);                               /* 清除 Line22 上的中断标志位 */
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);   /* 设置 Line22 上的中断，配置为上升沿触发中断 */
    nvic_irq_enable(RTC_WKUP_IRQn, 2, 2);                   /* 设置优先级，抢占优先级为 2，响应优先级为 2 */
}

/**
 * @brief       RTC 闹钟中断服务函数
 * @param       无
 * @retval      无
 */
void RTC_Alarm_IRQHandler(void)
{
    if (RESET != rtc_flag_get(RTC_FLAG_ALRM0)) /* ALARM0 中断? */
    {
        rtc_flag_clear(RTC_FLAG_ALRM0);        /* 清除中断标志 */       
        printf("ALARM0!\r\n");
    } 
    
    exti_flag_clear(EXTI_17);                  /* 清除中断线 17 的中断标志 */
}

/**
 * @brief       RTC WAKE UP 中断服务函数
 * @param       无
 * @retval      无
 */
void RTC_WKUP_IRQHandler(void)
{   
    if (RESET != rtc_flag_get(RTC_FLAG_WT))    /* WK_UP 中断? */   
    {       
        rtc_flag_clear(RTC_FLAG_WT);           /* 清除中断标志 */
        //LED1_TOGGLE();
    } 
    
    exti_flag_clear(EXTI_22);                  /* 清除中断线 22 的中断标志 */
}

void rtc_show_time(void)
{
//    uint32_t time_subsecond = 0;
//    uint8_t subsecond_ss = 0,subsecond_ts = 0,subsecond_hs = 0;

    rtc_current_time_get(&rtc_initpara);

    /* get the subsecond value of current time, and convert it into fractional format */
//    time_subsecond = rtc_subsecond_get();
//    subsecond_ss=(1000-(time_subsecond*1000+1000)/400)/100;
//    subsecond_ts=(1000-(time_subsecond*1000+1000)/400)%100/10;
//    subsecond_hs=(1000-(time_subsecond*1000+1000)/400)%10;

    printf("20%0.2x-%0.2x-%0.2x", \
           rtc_initpara.year, rtc_initpara.month, rtc_initpara.date);

    printf(" : %0.2x:%0.2x:%0.2x \r\n", \
           rtc_initpara.hour, rtc_initpara.minute, rtc_initpara.second);
}











