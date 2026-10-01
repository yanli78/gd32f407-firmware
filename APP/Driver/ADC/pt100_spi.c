#include "pt100_spi.h"
#include "systick.h"

/* ---------------------------------------------------------------------------
 * PT100 采集：AD3344（SPI0）读电阻 -> 查表校准 -> Callendar-Van Dusen 反解温度
 * ------------------------------------------------------------------------- */

/* 校准前的实测电阻与对应的实际电阻（原厂标定表，用于线性插值修正） */
typedef struct {
    float measured;     /* AD3344 实测电阻 */
    float actual;       /* 实际电阻 */
} pt100_cal_point_t;

static const pt100_cal_point_t pt100_cal_table[] = {
    {  80.306f,  80.60f },
    { 101.336f, 100.00f },
    { 112.900f, 111.85f },
    { 114.100f, 111.85f },
    { 115.061f, 115.00f },
    { 123.896f, 124.00f },
    { 130.212f, 130.00f },
    { 140.929f, 140.00f },
    { 142.138f, 150.00f },
    { 142.285f, 154.00f }
};

/* Pt100 分度常数：R = R0 * (1 + A*T + B*T^2 [+ C*(T-100)*T^3]) */
#define PT100_R0            100.0f
#define PT100_COEF_A        3.9083e-3f
#define PT100_COEF_B        (-5.775e-7f)
#define PT100_COEF_C        (-4.183e-12f)
#define PT100_TEMP_MIN      (-50.0f)        /* 二分搜索区间 */
#define PT100_TEMP_MAX      150.0f
#define PT100_SEARCH_ITER    24U            /* 24 次二分 -> 分辨率约 1.2e-5 ℃ */

/* 实测电压 -> 电阻 的线性拟合系数（原厂标定） */
#define PT100_FIT_SLOPE     13.7488f
#define PT100_FIT_OFFSET    97.6430f

/* AD3344 各档满量程输入范围，索引与 PT100_AD3344_PGA_xxx 一致 */
static const float ad3344_fsr_table[7] = {
    6.144f, 4.096f, 2.048f, 1.024f, 0.512f, 0.256f, 0.064f
};

/* 各数据速率所需的转换等待时间（ms，含余量），索引与 PT100_AD3344_DR_xxx 一致 */
static const uint16_t ad3344_conv_delay_ms[8] = {
    170U, 90U, 45U, 25U, 15U, 6U, 4U, 3U
};

/* 开路 / 短路的判定门限 */
#define PT100_OPEN_CIRCUIT_V    3.29f
#define PT100_SHORT_CIRCUIT_V   0.01f
#define PT100_OPEN_CIRCUIT_R    999999.0f

static float ad3344_fsr = 4.096f;
static uint8_t ad3344_input_mode = PT100_AD3344_SINGLE_END;
static uint8_t ad3344_channel = 0U;
static uint8_t ad3344_data_rate = PT100_AD3344_DR_6_25SPS;
static PT100_AD3344_PGA_TypeDef ad3344_pga_gain = PT100_AD3344_PGA_4_096V;

#define PT100_AD3344_OS_SINGLE         0x8000U
#define PT100_AD3344_MODE_SINGLE       0x0100U
#define PT100_AD3344_PULL_UP_EN        0x0008U
#define PT100_AD3344_NOP_VALID         0x0002U
#define PT100_AD3344_RESERVED_VALUE    0x0001U

/** @brief 取增益档对应的满量程电压，越界时按 4.096V 处理 */
static float ad3344_fsr_from_gain(PT100_AD3344_PGA_TypeDef pga_gain)
{
    if((uint32_t)pga_gain > 6U) return 4.096f;
    return ad3344_fsr_table[(uint32_t)pga_gain];
}

static uint8_t spi_read_write_byte(uint8_t data)
{
    while(RESET == spi_i2s_flag_get(PT100_AD3344_SPI_PERIPH, SPI_FLAG_TBE));
    spi_i2s_data_transmit(PT100_AD3344_SPI_PERIPH, data);
    while(RESET == spi_i2s_flag_get(PT100_AD3344_SPI_PERIPH, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(PT100_AD3344_SPI_PERIPH);
}

static uint16_t spi_read_write_word(uint16_t data)
{
    uint16_t receive_data;

    receive_data = (uint16_t)spi_read_write_byte((uint8_t)(data >> 8)) << 8;
    receive_data |= spi_read_write_byte((uint8_t)data);
    return receive_data;
}

static uint16_t ad3344_build_config(uint8_t input_mode, uint8_t channel, uint8_t data_rate, PT100_AD3344_PGA_TypeDef pga_gain)
{
    uint16_t config = PT100_AD3344_OS_SINGLE | PT100_AD3344_MODE_SINGLE |
                      PT100_AD3344_PULL_UP_EN | PT100_AD3344_NOP_VALID |
                      PT100_AD3344_RESERVED_VALUE;

    if(input_mode == PT100_AD3344_SINGLE_END){
        config |= (uint16_t)(0x4000U + ((channel & 0x03U) << 12));
    }else{
        config |= (uint16_t)((channel & 0x03U) << 12);
    }

    config |= (uint16_t)((pga_gain & 0x07U) << 9);
    config |= (uint16_t)((data_rate & 0x07U) << 5);
    return config;
}

static int16_t ad3344_transfer_config(uint16_t config)
{
    uint16_t val;

    PT100_AD3344_CS_LOW();
    val = spi_read_write_word(config);
    PT100_AD3344_CS_HIGH();

    return (int16_t)val;
}

/** @brief 按当前数据速率等待一次转换完成 */
static void ad3344_wait_conversion(void)
{
    uint8_t rate = (uint8_t)(ad3344_data_rate & 0x07U);
    delay_1ms(ad3344_conv_delay_ms[rate]);
}

static void ad3344_write_config(uint16_t config)
{
    (void)ad3344_transfer_config(config);
}

/** @brief 用 Callendar-Van Dusen 公式由温度算电阻（用于二分反解） */
static float pt100_temperature_to_resistance(float temperature)
{
    float t2 = temperature * temperature;

    if(temperature < 0.0f) {
        return PT100_R0 * (1.0f + PT100_COEF_A * temperature + PT100_COEF_B * t2
                           + PT100_COEF_C * (temperature - 100.0f) * t2 * temperature);
    }
    return PT100_R0 * (1.0f + PT100_COEF_A * temperature + PT100_COEF_B * t2);
}

/**
 * @brief  由电阻反解温度
 * @note   R(T) 在 -50~150℃ 区间单调递增，用二分法反解，
 *         比直接解三次方程更省事，24 次迭代精度远超传感器误差。
 */
static float pt100_resistance_to_temperature(float resistance)
{
    float low = PT100_TEMP_MIN;
    float high = PT100_TEMP_MAX;
    uint8_t i;

    for(i = 0U; i < PT100_SEARCH_ITER; i++){
        float mid = (low + high) * 0.5f;
        float rt = pt100_temperature_to_resistance(mid);

        if(rt < resistance) low = mid;
        else high = mid;
    }

    return (low + high) * 0.5f;
}

/** @brief 按标定表对实测电阻做分段线性修正 */
static float pt100_compensate_resistance(float measured_resistance)
{
    uint8_t i;
    const uint8_t count = (uint8_t)(sizeof(pt100_cal_table) / sizeof(pt100_cal_table[0]));

    if(measured_resistance <= pt100_cal_table[0].measured) return pt100_cal_table[0].actual;

    for(i = 1U; i < count; i++){
        if(measured_resistance <= pt100_cal_table[i].measured){
            float ratio = (measured_resistance - pt100_cal_table[i - 1U].measured) /
                          (pt100_cal_table[i].measured - pt100_cal_table[i - 1U].measured);
            return pt100_cal_table[i - 1U].actual +
                   ratio * (pt100_cal_table[i].actual - pt100_cal_table[i - 1U].actual);
        }
    }

    return pt100_cal_table[count - 1U].actual;
}

/** @brief 取三个采样值的中位数，滤掉单次跳变 */
static float pt100_median3(float a, float b, float c)
{
    if(a > b){ float t = a; a = b; b = t; }
    if(b > c){ float t = b; b = c; c = t; }
    if(a > b){ float t = a; a = b; b = t; }
    return b;
}

static void ad3344_spi_init(void)
{
    spi_parameter_struct spi_init_struct;

    rcu_periph_clock_enable(PT100_AD3344_SPI_RCU);
    rcu_periph_clock_enable(PT100_AD3344_SPI_GPIO_RCU);
    rcu_periph_clock_enable(PT100_AD3344_CS_RCU);

    gpio_af_set(PT100_AD3344_SPI_GPIO_PORT, PT100_AD3344_SPI_AF,
                PT100_AD3344_SCK_PIN | PT100_AD3344_MISO_PIN | PT100_AD3344_MOSI_PIN);
    gpio_mode_set(PT100_AD3344_SPI_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  PT100_AD3344_SCK_PIN | PT100_AD3344_MISO_PIN | PT100_AD3344_MOSI_PIN);
    gpio_output_options_set(PT100_AD3344_SPI_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            PT100_AD3344_SCK_PIN | PT100_AD3344_MISO_PIN | PT100_AD3344_MOSI_PIN);

    gpio_mode_set(PT100_AD3344_CS_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PT100_AD3344_CS_PIN);
    gpio_output_options_set(PT100_AD3344_CS_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PT100_AD3344_CS_PIN);
    PT100_AD3344_CS_HIGH();

    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_256;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(PT100_AD3344_SPI_PERIPH, &spi_init_struct);
    spi_enable(PT100_AD3344_SPI_PERIPH);
}

/**
 * @brief  初始化 PT100 采集通道（首次调用时顺带初始化 SPI0）
 * @param  input_mode: 单端 / 差分
 * @param  channel   : 通道号（0~3）
 * @param  data_rate : 数据速率
 * @param  pga_gain  : 增益档，决定满量程
 */
void pt100_ad3344_init(uint8_t input_mode, uint8_t channel, uint8_t data_rate, PT100_AD3344_PGA_TypeDef pga_gain)
{
    static uint8_t spi_inited = 0;
    if(!spi_inited){
        ad3344_spi_init();
        spi_inited = 1;
    }

    ad3344_input_mode = input_mode;
    ad3344_channel = channel & 0x03U;
    ad3344_data_rate = data_rate & 0x07U;
    ad3344_pga_gain = pga_gain;
    ad3344_fsr = ad3344_fsr_from_gain(pga_gain);

    ad3344_write_config(ad3344_build_config(ad3344_input_mode, ad3344_channel, ad3344_data_rate, ad3344_pga_gain));
}

int16_t pt100_ad3344_read_single(void)
{
    uint16_t config = ad3344_build_config(ad3344_input_mode, ad3344_channel, ad3344_data_rate, ad3344_pga_gain);

    ad3344_write_config(config);
    ad3344_wait_conversion();
    return ad3344_transfer_config(config);
}

float pt100_ad3344_to_voltage(int16_t adc_value)
{
    return (float)adc_value * ad3344_fsr / 32768.0f;
}

/**
 * @brief  读取 PT100 电阻
 * @param  channel     : 采集通道
 * @param  ref_resistor: 保留参数（当前实现用标定拟合，不参与计算）
 * @note   先做一次丢弃转换：切换通道/配置后的第一次转换尚未稳定，
 *         之后再取 3 次做中位数滤波。这段流程与原实现一致。
 */
float pt100_ad3344_read_resistance(uint8_t channel, float ref_resistor)
{
    float v0, v1, v2, v;

    (void)ref_resistor;

    if((ad3344_input_mode != PT100_AD3344_SINGLE_END) || ((channel & 0x03U) != ad3344_channel)){
        pt100_ad3344_init(PT100_AD3344_SINGLE_END, channel, ad3344_data_rate, ad3344_pga_gain);
    }

    (void)pt100_ad3344_read_single();                   /* 丢弃：配置切换后的第一次转换 */
    v0 = pt100_ad3344_to_voltage(pt100_ad3344_read_single());
    v1 = pt100_ad3344_to_voltage(pt100_ad3344_read_single());
    v2 = pt100_ad3344_to_voltage(pt100_ad3344_read_single());
    v = pt100_median3(v0, v1, v2);

    if(v >= PT100_OPEN_CIRCUIT_V) return PT100_OPEN_CIRCUIT_R;
    if(v <= PT100_SHORT_CIRCUIT_V) return 0.0f;
    return PT100_FIT_SLOPE * v + PT100_FIT_OFFSET;
}

float pt100_read_temperature(uint8_t channel, float ref_resistor)
{
    float Rpt = pt100_ad3344_read_resistance(channel, ref_resistor);
    return pt100_resistance_to_temperature(pt100_compensate_resistance(Rpt));
}
