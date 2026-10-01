#include "Boot_RS485.h"
#include "Boot_Timer.h"

/* 原始模式下超过该空闲时间就认为镜像传输结束（APP 侧为 1s） */
#define RS485_RAW_IDLE_MS   500U

static volatile uint8_t rx_buf[RS485_RX_BUF_LEN];
static volatile uint16_t rx_index = 0U;
static volatile uint8_t rx_done = 0U;
static volatile uint8_t raw_mode = 0U;
static volatile uint8_t raw_buf[RS485_RAW_BUF_LEN];
static volatile uint32_t raw_len = 0U;
static volatile uint8_t raw_done = 0U;
static volatile uint8_t raw_saved = 0U;
static volatile uint32_t raw_last_ms = 0U;

static void rs485_rx_enable(void)
{
    gpio_bit_reset(GPIOA, GPIO_PIN_1);
}

static void rs485_tx_enable(void)
{
    gpio_bit_set(GPIOA, GPIO_PIN_1);
}

void Boot_RS485_Init(uint32_t baudrate)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART1);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2 | GPIO_PIN_3);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_1);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    rs485_rx_enable();
    usart_deinit(USART1);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_baudrate_set(USART1, baudrate);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    nvic_irq_enable(USART1_IRQn, 0U, 0U);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
    usart_enable(USART1);
}

void Boot_RS485_SendString(const char *str)
{
    rs485_tx_enable();
    while(*str != '\0') {
        usart_data_transmit(USART1, (uint8_t)*str++);
        while(RESET == usart_flag_get(USART1, USART_FLAG_TBE)) {
        }
    }
    while(RESET == usart_flag_get(USART1, USART_FLAG_TC)) {
    }
    rs485_rx_enable();
}

void Boot_RS485_StartRaw(void)
{
    __disable_irq();
    raw_mode = 1U;
    raw_len = 0U;
    raw_done = 0U;
    raw_saved = 0U;
    raw_last_ms = Boot_Ms();
    __enable_irq();
}

/**
 * @brief  查询原始模式是否已收完一帧（升级镜像）
 * @retval 1 表示本次传输结束且尚未被取走；0 表示还没收完或已经取走过
 * @note   结束条件：缓冲写满，或空闲超过 RS485_RAW_IDLE_MS。
 *         raw_saved 保证同一批数据只上报一次。
 */
uint8_t Boot_RS485_RawFinished(void)
{
    if(raw_saved != 0U) return 0U;
    if((raw_done != 0U) || ((raw_len > 0U) && ((Boot_Ms() - raw_last_ms) >= RS485_RAW_IDLE_MS))) {
        __disable_irq();
        raw_mode = 0U;
        raw_done = 1U;
        raw_saved = 1U;
        __enable_irq();
        return 1U;
    }
    return 0U;
}

const volatile uint8_t *Boot_RS485_RawBuf(void)
{
    return raw_buf;
}

uint32_t Boot_RS485_RawLen(void)
{
    return raw_len;
}

uint8_t Boot_RS485_GetFrame(uint8_t *ascii, uint16_t *ascii_len)
{
    if(rx_done == 0U) return 0U;
    __disable_irq();
    *ascii_len = (uint16_t)strlen((const char *)rx_buf);
    memcpy(ascii, (const void *)rx_buf, *ascii_len + 1U);
    rx_done = 0U;
    __enable_irq();
    return 1U;
}

void USART1_IRQHandler(void)
{
    uint8_t ch;
    if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        ch = (uint8_t)usart_data_receive(USART1);
        if(raw_mode != 0U) {
            if(raw_done == 0U) {
                if(raw_len < RS485_RAW_BUF_LEN) raw_buf[raw_len++] = ch;
                raw_last_ms = Boot_Ms();
                if(raw_len >= RS485_RAW_BUF_LEN) raw_done = 1U;
            }
            return;
        }
        if(rx_index < (RS485_RX_BUF_LEN - 1U)) {
            if((ch == '\r') || (ch == '\n')) {
                if(rx_index > 0U) {
                    rx_buf[rx_index] = '\0';
                    rx_done = 1U;
                    rx_index = 0U;
                }
            } else {
                rx_buf[rx_index++] = ch;
                if(rx_index >= 4U) {
                    if((rx_buf[rx_index - 4U] == 'B') && (rx_buf[rx_index - 3U] == '6') && (rx_buf[rx_index - 2U] == 'A') && (rx_buf[rx_index - 1U] == '5')) {
                        rx_buf[rx_index] = '\0';
                        rx_done = 1U;
                        rx_index = 0U;
                    }
                }
            }
        } else {
            rx_index = 0U;
        }
    }
}
