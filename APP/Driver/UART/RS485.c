#include "Head.h"

#define RS485_USART                 USART1
#define RS485_USART_RCU             RCU_USART1
#define RS485_GPIO_RCU              RCU_GPIOA
#define RS485_GPIO_PORT             GPIOA
#define RS485_TX_PIN                GPIO_PIN_2
#define RS485_RX_PIN                GPIO_PIN_3
#define RS485_DE_RCU                RCU_GPIOA
#define RS485_DE_PORT               GPIOA
#define RS485_DE_PIN                GPIO_PIN_1
#define RS485_AF                    GPIO_AF_7

volatile uint8_t rs485_rx_buf[RS485_RX_BUF_LEN];
volatile uint16_t rs485_rx_index = 0;
volatile uint8_t rs485_rx_done = 0;
volatile uint8_t rs485_raw_mode = 0;
volatile uint8_t rs485_raw_buf[RS485_RAW_BUF_LEN];
volatile uint32_t rs485_raw_len = 0;
volatile uint8_t rs485_raw_done = 0;
volatile uint32_t rs485_raw_last_s = 0;

static void RS485_TxEnable(void)
{
    gpio_bit_set(RS485_DE_PORT, RS485_DE_PIN);
}

static void RS485_RxEnable(void)
{
    gpio_bit_reset(RS485_DE_PORT, RS485_DE_PIN);
}

void RS485_Config(uint32_t baudrate)
{
    rcu_periph_clock_enable(RS485_GPIO_RCU);
    rcu_periph_clock_enable(RS485_DE_RCU);
    rcu_periph_clock_enable(RS485_USART_RCU);

    gpio_af_set(RS485_GPIO_PORT, RS485_AF, RS485_TX_PIN | RS485_RX_PIN);
    gpio_mode_set(RS485_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, RS485_TX_PIN | RS485_RX_PIN);
    gpio_output_options_set(RS485_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_TX_PIN | RS485_RX_PIN);

    gpio_mode_set(RS485_DE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RS485_DE_PIN);
    gpio_output_options_set(RS485_DE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, RS485_DE_PIN);
    RS485_RxEnable();

    usart_deinit(RS485_USART);
    usart_word_length_set(RS485_USART, USART_WL_8BIT);
    usart_stop_bit_set(RS485_USART, USART_STB_1BIT);
    usart_parity_config(RS485_USART, USART_PM_NONE);
    usart_baudrate_set(RS485_USART, baudrate);
    usart_receive_config(RS485_USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(RS485_USART, USART_TRANSMIT_ENABLE);
    nvic_irq_enable(USART1_IRQn, 0, 0);
    usart_interrupt_enable(RS485_USART, USART_INT_RBNE);
    usart_enable(RS485_USART);
}

void RS485_SetBaudrate(uint32_t baudrate)
{
    usart_disable(RS485_USART);
    usart_baudrate_set(RS485_USART, baudrate);
    usart_enable(RS485_USART);
}

void RS485_SendChar(uint8_t ch)
{
    RS485_TxEnable();
    usart_data_transmit(RS485_USART, ch);
    while(RESET == usart_flag_get(RS485_USART, USART_FLAG_TBE)) {
    }
    while(RESET == usart_flag_get(RS485_USART, USART_FLAG_TC)) {
    }
    RS485_RxEnable();
}

void RS485_SendString(const char *str)
{
    RS485_TxEnable();
    while('\0' != *str) {
        usart_data_transmit(RS485_USART, (uint8_t)*str++);
        while(RESET == usart_flag_get(RS485_USART, USART_FLAG_TBE)) {
        }
    }
    while(RESET == usart_flag_get(RS485_USART, USART_FLAG_TC)) {
    }
    RS485_RxEnable();
}

void RS485_IRQHandler(void)
{
    uint8_t ch;

    if(RESET != usart_interrupt_flag_get(RS485_USART, USART_INT_FLAG_RBNE)) {
        ch = (uint8_t)usart_data_receive(RS485_USART);
        if(rs485_raw_mode != 0U) {
            if(rs485_raw_done == 0U) {
                if(rs485_raw_len < RS485_RAW_BUF_LEN) {
                    rs485_raw_buf[rs485_raw_len++] = ch;
                    rs485_raw_last_s = s;
                } else {
                    rs485_raw_done = 1U;
                }
            }
            return;
        }
        if(rs485_rx_index < (RS485_RX_BUF_LEN - 1U)) {
            if((ch == '\r') || (ch == '\n')) {
                if(rs485_rx_index > 0U) {
                    rs485_rx_buf[rs485_rx_index] = '\0';
                    rs485_rx_done = 1U;
                    rs485_rx_index = 0U;
                }
            } else {
                rs485_rx_buf[rs485_rx_index++] = ch;
                if(rs485_rx_index >= 4U) {
                    if((rs485_rx_buf[rs485_rx_index - 4U] == 'B') && (rs485_rx_buf[rs485_rx_index - 3U] == '6') && (rs485_rx_buf[rs485_rx_index - 2U] == 'A') && (rs485_rx_buf[rs485_rx_index - 1U] == '5')) {
                        rs485_rx_buf[rs485_rx_index] = '\0';
                        rs485_rx_done = 1U;
                        rs485_rx_index = 0U;
                    }
                }
            }
        } else {
            rs485_rx_index = 0U;
        }
    }
}
