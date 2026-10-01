.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.global g_pfnVectors
.global Default_Handler

.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
copy_data:
    cmp r0, r1
    bcc copy_word
    b clear_bss
copy_word:
    ldr r3, [r2], #4
    str r3, [r0], #4
    b copy_data
clear_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
zero_bss:
    cmp r0, r1
    bcc zero_word
    b call_main
zero_word:
    str r2, [r0], #4
    b zero_bss
call_main:
    bl SystemInit
    bl __libc_init_array
    bl main
loop:
    b loop

.section .text.Default_Handler
.type Default_Handler, %function
Default_Handler:
    b Default_Handler

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
weak_handler TIMER2_IRQHandler
weak_handler USART0_IRQHandler
weak_handler USART1_IRQHandler
weak_handler SDIO_IRQHandler

.section .isr_vector,"a",%progbits
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .rept 29
    .word Default_Handler
    .endr
    .word TIMER2_IRQHandler
    .rept 7
    .word Default_Handler
    .endr
    .word USART0_IRQHandler
    .word USART1_IRQHandler
    .rept 10
    .word Default_Handler
    .endr
    .word SDIO_IRQHandler
    .rept 66
    .word Default_Handler
    .endr
