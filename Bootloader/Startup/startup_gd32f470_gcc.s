/*!
    \file    startup_gd32f470_gcc.s
    \brief   GNU as 启动文件（Cortex-M4F, GD32F470VE）

    \note    向量表顺序与厂商 startup_gd32f450_470.s 逐项一致，共 107 个表项，
             所有中断均为弱定义，工程里的同名强符号会自动覆盖。
             原文件的向量表只显式列出了 TIMER2/USART0/USART1/SDIO，
             RTC_WKUP（IRQ3）等落在 .rept 填充区里指向 Default_Handler，
             会导致 App_Shui10s() 深度睡眠被 RTC 唤醒后跳进死循环。
*/

.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

/* ------------------------------------------------------------------ 中断向量表 */
.section .isr_vector,"a",%progbits
.global g_pfnVectors
.type g_pfnVectors, %object
g_pfnVectors:
    .word _estack                         /* 0: initial stack pointer */
    .word Reset_Handler                   /* Reset Handler */
    .word NMI_Handler                     /* NMI Handler */
    .word HardFault_Handler               /* Hard Fault Handler */
    .word MemManage_Handler               /* MPU Fault Handler */
    .word BusFault_Handler                /* Bus Fault Handler */
    .word UsageFault_Handler              /* Usage Fault Handler */
    .word 0                               /* Reserved */
    .word 0                               /* Reserved */
    .word 0                               /* Reserved */
    .word 0                               /* Reserved */
    .word SVC_Handler                     /* SVCall Handler */
    .word DebugMon_Handler                /* Debug Monitor Handler */
    .word 0                               /* Reserved */
    .word PendSV_Handler                  /* PendSV Handler */
    .word SysTick_Handler                 /* SysTick Handler */
    .word WWDGT_IRQHandler                /* 16:Window Watchdog Timer */
    .word LVD_IRQHandler                  /* 17:LVD through EXTI Line detect */
    .word TAMPER_STAMP_IRQHandler         /* 18:Tamper and TimeStamp through EXTI Line detect */
    .word RTC_WKUP_IRQHandler             /* 19:RTC Wakeup through EXTI Line */
    .word FMC_IRQHandler                  /* 20:FMC */
    .word RCU_CTC_IRQHandler              /* 21:RCU and CTC */
    .word EXTI0_IRQHandler                /* 22:EXTI Line 0 */
    .word EXTI1_IRQHandler                /* 23:EXTI Line 1 */
    .word EXTI2_IRQHandler                /* 24:EXTI Line 2 */
    .word EXTI3_IRQHandler                /* 25:EXTI Line 3 */
    .word EXTI4_IRQHandler                /* 26:EXTI Line 4 */
    .word DMA0_Channel0_IRQHandler        /* 27:DMA0 Channel0 */
    .word DMA0_Channel1_IRQHandler        /* 28:DMA0 Channel1 */
    .word DMA0_Channel2_IRQHandler        /* 29:DMA0 Channel2 */
    .word DMA0_Channel3_IRQHandler        /* 30:DMA0 Channel3 */
    .word DMA0_Channel4_IRQHandler        /* 31:DMA0 Channel4 */
    .word DMA0_Channel5_IRQHandler        /* 32:DMA0 Channel5 */
    .word DMA0_Channel6_IRQHandler        /* 33:DMA0 Channel6 */
    .word ADC_IRQHandler                  /* 34:ADC */
    .word CAN0_TX_IRQHandler              /* 35:CAN0 TX */
    .word CAN0_RX0_IRQHandler             /* 36:CAN0 RX0 */
    .word CAN0_RX1_IRQHandler             /* 37:CAN0 RX1 */
    .word CAN0_EWMC_IRQHandler            /* 38:CAN0 EWMC */
    .word EXTI5_9_IRQHandler              /* 39:EXTI5 to EXTI9 */
    .word TIMER0_BRK_TIMER8_IRQHandler    /* 40:TIMER0 Break and TIMER8 */
    .word TIMER0_UP_TIMER9_IRQHandler     /* 41:TIMER0 Update and TIMER9 */
    .word TIMER0_TRG_CMT_TIMER10_IRQHandler/* 42:TIMER0 Trigger and Commutation and TIMER10 */
    .word TIMER0_Channel_IRQHandler       /* 43:TIMER0 Capture Compare */
    .word TIMER1_IRQHandler               /* 44:TIMER1 */
    .word TIMER2_IRQHandler               /* 45:TIMER2 */
    .word TIMER3_IRQHandler               /* 46:TIMER3 */
    .word I2C0_EV_IRQHandler              /* 47:I2C0 Event */
    .word I2C0_ER_IRQHandler              /* 48:I2C0 Error */
    .word I2C1_EV_IRQHandler              /* 49:I2C1 Event */
    .word I2C1_ER_IRQHandler              /* 50:I2C1 Error */
    .word SPI0_IRQHandler                 /* 51:SPI0 */
    .word SPI1_IRQHandler                 /* 52:SPI1 */
    .word USART0_IRQHandler               /* 53:USART0 */
    .word USART1_IRQHandler               /* 54:USART1 */
    .word USART2_IRQHandler               /* 55:USART2 */
    .word EXTI10_15_IRQHandler            /* 56:EXTI10 to EXTI15 */
    .word RTC_Alarm_IRQHandler            /* 57:RTC Alarm */
    .word USBFS_WKUP_IRQHandler           /* 58:USBFS Wakeup */
    .word TIMER7_BRK_TIMER11_IRQHandler   /* 59:TIMER7 Break and TIMER11 */
    .word TIMER7_UP_TIMER12_IRQHandler    /* 60:TIMER7 Update and TIMER12 */
    .word TIMER7_TRG_CMT_TIMER13_IRQHandler/* 61:TIMER7 Trigger and Commutation and TIMER13 */
    .word TIMER7_Channel_IRQHandler       /* 62:TIMER7 Channel Capture Compare */
    .word DMA0_Channel7_IRQHandler        /* 63:DMA0 Channel7 */
    .word EXMC_IRQHandler                 /* 64:EXMC */
    .word SDIO_IRQHandler                 /* 65:SDIO */
    .word TIMER4_IRQHandler               /* 66:TIMER4 */
    .word SPI2_IRQHandler                 /* 67:SPI2 */
    .word UART3_IRQHandler                /* 68:UART3 */
    .word UART4_IRQHandler                /* 69:UART4 */
    .word TIMER5_DAC_IRQHandler           /* 70:TIMER5 and DAC0 DAC1 Underrun error */
    .word TIMER6_IRQHandler               /* 71:TIMER6 */
    .word DMA1_Channel0_IRQHandler        /* 72:DMA1 Channel0 */
    .word DMA1_Channel1_IRQHandler        /* 73:DMA1 Channel1 */
    .word DMA1_Channel2_IRQHandler        /* 74:DMA1 Channel2 */
    .word DMA1_Channel3_IRQHandler        /* 75:DMA1 Channel3 */
    .word DMA1_Channel4_IRQHandler        /* 76:DMA1 Channel4 */
    .word ENET_IRQHandler                 /* 77:Ethernet */
    .word ENET_WKUP_IRQHandler            /* 78:Ethernet Wakeup through EXTI Line */
    .word CAN1_TX_IRQHandler              /* 79:CAN1 TX */
    .word CAN1_RX0_IRQHandler             /* 80:CAN1 RX0 */
    .word CAN1_RX1_IRQHandler             /* 81:CAN1 RX1 */
    .word CAN1_EWMC_IRQHandler            /* 82:CAN1 EWMC */
    .word USBFS_IRQHandler                /* 83:USBFS */
    .word DMA1_Channel5_IRQHandler        /* 84:DMA1 Channel5 */
    .word DMA1_Channel6_IRQHandler        /* 85:DMA1 Channel6 */
    .word DMA1_Channel7_IRQHandler        /* 86:DMA1 Channel7 */
    .word USART5_IRQHandler               /* 87:USART5 */
    .word I2C2_EV_IRQHandler              /* 88:I2C2 Event */
    .word I2C2_ER_IRQHandler              /* 89:I2C2 Error */
    .word USBHS_EP1_Out_IRQHandler        /* 90:USBHS Endpoint 1 Out */
    .word USBHS_EP1_In_IRQHandler         /* 91:USBHS Endpoint 1 in */
    .word USBHS_WKUP_IRQHandler           /* 92:USBHS Wakeup through EXTI Line */
    .word USBHS_IRQHandler                /* 93:USBHS */
    .word DCI_IRQHandler                  /* 94:DCI */
    .word 0                               /* 95:Reserved */
    .word TRNG_IRQHandler                 /* 96:TRNG */
    .word FPU_IRQHandler                  /* 97:FPU */
    .word UART6_IRQHandler                /* 98:UART6 */
    .word UART7_IRQHandler                /* 99:UART7 */
    .word SPI3_IRQHandler                 /* 100:SPI3 */
    .word SPI4_IRQHandler                 /* 101:SPI4 */
    .word SPI5_IRQHandler                 /* 102:SPI5 */
    .word 0                               /* 103:Reserved */
    .word TLI_IRQHandler                  /* 104:TLI */
    .word TLI_ER_IRQHandler               /* 105:TLI Error */
    .word IPA_IRQHandler                  /* 106:IPA */
.size g_pfnVectors, .-g_pfnVectors

/* -------------------------------------------------------------------- 复位入口 */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0

    /* 把 .data 段从 Flash 搬到 RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    b 1f
0:  ldr r3, [r2], #4
    str r3, [r0], #4
1:  cmp r0, r1
    bcc 0b

    /* 清零 .bss 段 */
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
    b 3f
2:  str r2, [r0], #4
3:  cmp r0, r1
    bcc 2b

    /* SystemInit（配置时钟并使能 FPU）-> C 运行时初始化 -> main */
    bl SystemInit
    bl __libc_init_array
    bl main
4:  b 4b
.size Reset_Handler, .-Reset_Handler

/* ---------------------------------------------------------------- 默认中断处理 */
.section .text.Default_Handler
.type Default_Handler, %function
Default_Handler:
    b Default_Handler
.size Default_Handler, .-Default_Handler

/* 所有中断弱定义到 Default_Handler（工程中同名函数会覆盖） */
.macro weak_handler name
    .weak \name
    .thumb_set \name, Default_Handler
.endm

weak_handler NMI_Handler
weak_handler HardFault_Handler
weak_handler MemManage_Handler
weak_handler BusFault_Handler
weak_handler UsageFault_Handler
weak_handler SVC_Handler
weak_handler DebugMon_Handler
weak_handler PendSV_Handler
weak_handler SysTick_Handler
weak_handler WWDGT_IRQHandler
weak_handler LVD_IRQHandler
weak_handler TAMPER_STAMP_IRQHandler
weak_handler RTC_WKUP_IRQHandler
weak_handler FMC_IRQHandler
weak_handler RCU_CTC_IRQHandler
weak_handler EXTI0_IRQHandler
weak_handler EXTI1_IRQHandler
weak_handler EXTI2_IRQHandler
weak_handler EXTI3_IRQHandler
weak_handler EXTI4_IRQHandler
weak_handler DMA0_Channel0_IRQHandler
weak_handler DMA0_Channel1_IRQHandler
weak_handler DMA0_Channel2_IRQHandler
weak_handler DMA0_Channel3_IRQHandler
weak_handler DMA0_Channel4_IRQHandler
weak_handler DMA0_Channel5_IRQHandler
weak_handler DMA0_Channel6_IRQHandler
weak_handler ADC_IRQHandler
weak_handler CAN0_TX_IRQHandler
weak_handler CAN0_RX0_IRQHandler
weak_handler CAN0_RX1_IRQHandler
weak_handler CAN0_EWMC_IRQHandler
weak_handler EXTI5_9_IRQHandler
weak_handler TIMER0_BRK_TIMER8_IRQHandler
weak_handler TIMER0_UP_TIMER9_IRQHandler
weak_handler TIMER0_TRG_CMT_TIMER10_IRQHandler
weak_handler TIMER0_Channel_IRQHandler
weak_handler TIMER1_IRQHandler
weak_handler TIMER2_IRQHandler
weak_handler TIMER3_IRQHandler
weak_handler I2C0_EV_IRQHandler
weak_handler I2C0_ER_IRQHandler
weak_handler I2C1_EV_IRQHandler
weak_handler I2C1_ER_IRQHandler
weak_handler SPI0_IRQHandler
weak_handler SPI1_IRQHandler
weak_handler USART0_IRQHandler
weak_handler USART1_IRQHandler
weak_handler USART2_IRQHandler
weak_handler EXTI10_15_IRQHandler
weak_handler RTC_Alarm_IRQHandler
weak_handler USBFS_WKUP_IRQHandler
weak_handler TIMER7_BRK_TIMER11_IRQHandler
weak_handler TIMER7_UP_TIMER12_IRQHandler
weak_handler TIMER7_TRG_CMT_TIMER13_IRQHandler
weak_handler TIMER7_Channel_IRQHandler
weak_handler DMA0_Channel7_IRQHandler
weak_handler EXMC_IRQHandler
weak_handler SDIO_IRQHandler
weak_handler TIMER4_IRQHandler
weak_handler SPI2_IRQHandler
weak_handler UART3_IRQHandler
weak_handler UART4_IRQHandler
weak_handler TIMER5_DAC_IRQHandler
weak_handler TIMER6_IRQHandler
weak_handler DMA1_Channel0_IRQHandler
weak_handler DMA1_Channel1_IRQHandler
weak_handler DMA1_Channel2_IRQHandler
weak_handler DMA1_Channel3_IRQHandler
weak_handler DMA1_Channel4_IRQHandler
weak_handler ENET_IRQHandler
weak_handler ENET_WKUP_IRQHandler
weak_handler CAN1_TX_IRQHandler
weak_handler CAN1_RX0_IRQHandler
weak_handler CAN1_RX1_IRQHandler
weak_handler CAN1_EWMC_IRQHandler
weak_handler USBFS_IRQHandler
weak_handler DMA1_Channel5_IRQHandler
weak_handler DMA1_Channel6_IRQHandler
weak_handler DMA1_Channel7_IRQHandler
weak_handler USART5_IRQHandler
weak_handler I2C2_EV_IRQHandler
weak_handler I2C2_ER_IRQHandler
weak_handler USBHS_EP1_Out_IRQHandler
weak_handler USBHS_EP1_In_IRQHandler
weak_handler USBHS_WKUP_IRQHandler
weak_handler USBHS_IRQHandler
weak_handler DCI_IRQHandler
weak_handler TRNG_IRQHandler
weak_handler FPU_IRQHandler
weak_handler UART6_IRQHandler
weak_handler UART7_IRQHandler
weak_handler SPI3_IRQHandler
weak_handler SPI4_IRQHandler
weak_handler SPI5_IRQHandler
weak_handler TLI_IRQHandler
weak_handler TLI_ER_IRQHandler
weak_handler IPA_IRQHandler

/* ----------------------------------------------------------------------- END */

