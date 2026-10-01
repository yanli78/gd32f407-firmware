#include "Head.h"

#define PROTO_START                  0xA5B6U
#define PROTO_END                    0xB6A5U
#define PROTO_VERSION                0x02U
#define PROTO_TYPE_CMD               0x01U
#define PROTO_TYPE_RESP              0x02U
#define PROTO_TYPE_HEART             0x05U
#define PROTO_TYPE_ERROR             0xFFU

#define CMD_REBOOT                   0x0101U
#define CMD_VERSION                  0x0104U
#define CMD_SET_TIME                 0x0105U
#define CMD_GET_TIME                 0x0106U
#define CMD_SET_ID                   0x01A1U
#define CMD_SET_BAUD                 0x01A2U
#define CMD_GET_ID                   0x0111U
#define CMD_GET_BAUD                 0x0112U
#define CMD_CH0                      0x0201U
#define CMD_CH1                      0x0202U
#define CMD_CH2                      0x0221U
#define CMD_SET_RATIO0               0x0241U
#define CMD_SET_RATIO1               0x0242U
#define CMD_SET_INTERVAL             0x0261U
#define CMD_SET_DAC                  0x0301U
#define CMD_AUTO_START               0x0302U
#define CMD_AUTO_STOP                0x0303U
#define CMD_SLEEP                    0x03AAU
#define CMD_GET_LIMIT_ALL            0x0400U
#define CMD_GET_LIMIT0               0x0401U
#define CMD_GET_LIMIT1               0x0402U
#define CMD_GET_LIMIT2               0x0403U
#define CMD_SET_LIMIT0               0x0411U
#define CMD_SET_LIMIT1               0x0412U
#define CMD_SET_LIMIT2               0x0413U
#define CMD_UPGRADE_REQ              0x0501U
#define CMD_UPGRADE_PREP             0x0502U
#define CMD_UPGRADE_EXEC             0x0503U
#define CMD_ALARM_ENABLE             0x0601U
#define CMD_ALARM_QUERY              0x0602U
#define CMD_ALARM_CLEAR              0x0603U
#define CMD_HEARTBEAT                0x8888U
#define CMD_FIND                     0xFFFFU
#define CMD_ERROR                    0xEEEEU
#define PROTO_PARAM_MAGIC            0x50415241U
#define PROTO_ALARM_MAGIC            0x414C4D31U
#define PROTO_UPGRADE_MAGIC          0x5AA5C33CU
#define PROTO_PARAM_ADDR             0x020000U
#define PROTO_ALARM_ADDR             0x021000U
#define PROTO_APP_ADDR               0x08011000U
#define PROTO_BACKUP_ADDR            0x08031000U
#define PROTO_TEMP_ADDR              0x08051000U
#define PROTO_SLOT_SIZE              0x00020000U
#define PROTO_BOOT_FLAG_ADDR          0x08010000U
#define PROTO_BOOT_FLAG_MAGIC         0x424F4F54U

typedef struct {
    uint32_t magic;
    uint16_t device_id;
    uint16_t baud_code;
    uint32_t baudrate;
} BootFlagTypeDef;

typedef struct {
    uint32_t magic;
    uint16_t device_id;
    uint8_t baud_code;
    uint32_t baudrate;
    float ch0_ratio;
    float ch1_ratio;
    float ch0_limit;
    float ch1_limit;
    float ch2_limit;
    uint8_t interval_code;
    uint32_t interval_s;
    uint8_t alarm_enable;
    uint8_t auto_report;
    uint32_t last_report_s;
    uint16_t dac_value;
    uint16_t reserved;
} ProtocolConfigTypeDef;

typedef struct {
    char text[80];
} AlarmRecordTypeDef;

typedef struct {
    uint32_t magic;
    uint8_t count;
    uint8_t head;
    uint16_t reserved;
    AlarmRecordTypeDef records[10];
} AlarmStoreTypeDef;

static ProtocolConfigTypeDef proto_cfg;
static AlarmRecordTypeDef alarm_records[10];
static uint8_t alarm_count = 0U;
static uint8_t alarm_head = 0U;
static uint32_t utc_base = 1780838400U;
static uint32_t utc_base_tick = 0U;
static uint8_t upgrade_ready = 0U;
static uint32_t upgrade_size = 0U;
static uint8_t baojing_biaoji = 0U;

static uint32_t baud_from_code(uint8_t code);
static uint32_t interval_from_code(uint8_t code);
static uint32_t get_u32(const uint8_t *buf);
static uint8_t flash_erase_range(uint32_t addr, uint32_t size);
static uint8_t flash_write_bytes(uint32_t addr, const uint8_t *data, uint32_t size);

static uint8_t flash_erase_range(uint32_t addr, uint32_t size)
{
    uint32_t end = addr + size;

    fmc_unlock();
    while(addr < end) {
        if(fmc_page_erase(addr) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
        addr += 4096U;
    }
    fmc_lock();
    return 1U;
}

static uint8_t flash_write_bytes(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i;

    fmc_unlock();
    for(i = 0U; i < size; i++) {
        if(fmc_byte_program(addr + i, data[i]) != FMC_READY) {
            fmc_lock();
            return 0U;
        }
    }
    fmc_lock();
    return 1U;
}

static uint8_t upgrade_store(void)
{
    if(rs485_raw_len <= 4U) return 0U;
    if(get_u32((const uint8_t *)rs485_raw_buf) != PROTO_UPGRADE_MAGIC) return 0U;
    upgrade_size = rs485_raw_len - 4U;
    if(upgrade_size > PROTO_SLOT_SIZE) return 0U;
    if(flash_erase_range(PROTO_TEMP_ADDR, PROTO_SLOT_SIZE) == 0U) return 0U;
    if(flash_write_bytes(PROTO_TEMP_ADDR, (const uint8_t *)&rs485_raw_buf[4], upgrade_size) == 0U) return 0U;
    upgrade_ready = 1U;
    return 1U;
}

static uint8_t upgrade_execute(void)
{
    BootFlagTypeDef boot_flag;

    if(upgrade_ready == 0U) return 0U;
    if(upgrade_size == 0U) return 0U;
    boot_flag.magic = PROTO_BOOT_FLAG_MAGIC;
    boot_flag.device_id = proto_cfg.device_id;
    boot_flag.baud_code = proto_cfg.baud_code;
    boot_flag.baudrate = proto_cfg.baudrate;
    if(flash_erase_range(PROTO_BOOT_FLAG_ADDR, 4096U) == 0U) return 0U;
    if(flash_write_bytes(PROTO_BOOT_FLAG_ADDR, (const uint8_t *)&boot_flag, sizeof(boot_flag)) == 0U) return 0U;
    return 1U;
}

static uint8_t is_leap(uint16_t year)
{
    if((year % 400U) == 0U) return 1U;
    if((year % 100U) == 0U) return 0U;
    if((year % 4U) == 0U) return 1U;
    return 0U;
}

static void format_time(uint32_t utc, char *buf, uint16_t len)
{
    static const uint8_t days_in_month[12] = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };
    uint32_t days = utc / 86400U;
    uint32_t secs = utc % 86400U;
    uint16_t year = 1970U;
    uint8_t month = 1U;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    while(1) {
        uint16_t yd = is_leap(year) ? 366U : 365U;
        if(days < yd) break;
        days -= yd;
        year++;
    }

    while(month <= 12U) {
        uint8_t md = days_in_month[month - 1U];
        if((month == 2U) && (is_leap(year) != 0U)) md++;
        if(days < md) break;
        days -= md;
        month++;
    }

    day = (uint8_t)(days + 1U);
    hour = (uint8_t)(secs / 3600U);
    secs %= 3600U;
    minute = (uint8_t)(secs / 60U);
    second = (uint8_t)(secs % 60U);
    snprintf(buf, len, "%04u-%02u-%02u %02u:%02u:%02u", year, month, day, hour, minute, second);
}

static void alarm_save(void)
{
    AlarmStoreTypeDef store;

    store.magic = PROTO_ALARM_MAGIC;
    store.count = alarm_count;
    store.head = alarm_head;
    store.reserved = 0U;
    memcpy(store.records, alarm_records, sizeof(alarm_records));
    spi_flash_sector_erase(PROTO_ALARM_ADDR);
    spi_flash_buffer_write((uint8_t *)&store, PROTO_ALARM_ADDR, sizeof(store));
}

static void alarm_load(void)
{
    AlarmStoreTypeDef store;

    spi_flash_buffer_read((uint8_t *)&store, PROTO_ALARM_ADDR, sizeof(store));
    if((store.magic == PROTO_ALARM_MAGIC) && (store.count <= 10U) && (store.head < 10U)) {
        alarm_count = store.count;
        alarm_head = store.head;
        memcpy(alarm_records, store.records, sizeof(alarm_records));
    }
}

static void protocol_defaults(void)
{
    proto_cfg.magic = PROTO_PARAM_MAGIC;
    proto_cfg.device_id = PROTO_DEVICE_ID_DEFAULT;
    proto_cfg.baud_code = 0x13U;
    proto_cfg.baudrate = 19200U;
    proto_cfg.ch0_ratio = 1.0f;
    proto_cfg.ch1_ratio = 1.0f;
    proto_cfg.ch0_limit = 3.0f;
    proto_cfg.ch1_limit = 3.0f;
    proto_cfg.ch2_limit = 150.0f;
    proto_cfg.interval_code = 0x01U;
    proto_cfg.interval_s = 1U;
    proto_cfg.alarm_enable = 1U;
    proto_cfg.auto_report = 0U;
    proto_cfg.last_report_s = 0U;
    proto_cfg.dac_value = 0U;
    proto_cfg.reserved = 0U;
}

static void Canshu_Baocun(void)
{
    ProtocolConfigTypeDef save_cfg = proto_cfg;
    save_cfg.magic = PROTO_PARAM_MAGIC;
    save_cfg.auto_report = 0U;
    save_cfg.last_report_s = 0U;
    save_cfg.baudrate = baud_from_code(save_cfg.baud_code);
    save_cfg.interval_s = interval_from_code(save_cfg.interval_code);
    spi_flash_sector_erase(PROTO_PARAM_ADDR);
    spi_flash_buffer_write((uint8_t *)&save_cfg, PROTO_PARAM_ADDR, sizeof(save_cfg));
}

static void Canshu_Duqu(void)
{
    ProtocolConfigTypeDef load_cfg;
    spi_flash_buffer_read((uint8_t *)&load_cfg, PROTO_PARAM_ADDR, sizeof(load_cfg));
    if((load_cfg.magic == PROTO_PARAM_MAGIC) && (load_cfg.device_id != 0U) && (load_cfg.device_id != 0xFFFFU)) {
        proto_cfg = load_cfg;
        proto_cfg.baudrate = baud_from_code(proto_cfg.baud_code);
        proto_cfg.interval_s = interval_from_code(proto_cfg.interval_code);
        proto_cfg.auto_report = 0U;
        proto_cfg.last_report_s = 0U;
    }
}

static uint8_t hex_value(uint8_t ch)
{
    if((ch >= '0') && (ch <= '9')) return ch - '0';
    if((ch >= 'A') && (ch <= 'F')) return ch - 'A' + 10U;
    if((ch >= 'a') && (ch <= 'f')) return ch - 'a' + 10U;
    return 0xFFU;
}

static char hex_char(uint8_t val)
{
    val &= 0x0FU;
    return (val < 10U) ? (char)('0' + val) : (char)('A' + val - 10U);
}

static uint8_t ascii_to_bytes(const volatile uint8_t *ascii, uint16_t ascii_len, uint8_t *out, uint16_t out_cap, uint16_t *out_len)
{
    uint16_t i;
    uint8_t h, l;

    if((ascii_len & 1U) != 0U) return 0U;
    *out_len = ascii_len / 2U;
    if(*out_len > out_cap) return 0U;
    for(i = 0U; i < *out_len; i++) {
        h = hex_value(ascii[i * 2U]);
        l = hex_value(ascii[i * 2U + 1U]);
        if((h > 0x0FU) || (l > 0x0FU)) return 0U;
        out[i] = (uint8_t)((h << 4U) | l);
    }
    return 1U;
}

static uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint8_t j;

    for(i = 0U; i < len; i++) {
        crc ^= data[i];
        for(j = 0U; j < 8U; j++) {
            if((crc & 0x0001U) != 0U) crc = (crc >> 1U) ^ 0xA001U;
            else crc >>= 1U;
        }
    }
    return crc;
}

static void bytes_to_ascii_send(const uint8_t *data, uint16_t len)
{
    char out[256];
    uint16_t i;

    if((len * 2U) >= sizeof(out)) return;
    for(i = 0U; i < len; i++) {
        out[i * 2U] = hex_char(data[i] >> 4U);
        out[i * 2U + 1U] = hex_char(data[i]);
    }
    out[len * 2U] = '\0';
    RS485_SendString(out);
}

static void put_u16(uint8_t *buf, uint16_t *idx, uint16_t v)
{
    buf[(*idx)++] = (uint8_t)(v >> 8U);
    buf[(*idx)++] = (uint8_t)v;
}

static void put_u32(uint8_t *buf, uint16_t *idx, uint32_t v)
{
    buf[(*idx)++] = (uint8_t)(v >> 24U);
    buf[(*idx)++] = (uint8_t)(v >> 16U);
    buf[(*idx)++] = (uint8_t)(v >> 8U);
    buf[(*idx)++] = (uint8_t)v;
}

static uint16_t get_u16(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8U) | buf[1];
}

static uint32_t get_u32(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24U) | ((uint32_t)buf[1] << 16U) | ((uint32_t)buf[2] << 8U) | buf[3];
}

static void put_float(uint8_t *buf, uint16_t *idx, float v)
{
    union { float f; uint8_t b[4]; } u;
    u.f = v;
    buf[(*idx)++] = u.b[3];
    buf[(*idx)++] = u.b[2];
    buf[(*idx)++] = u.b[1];
    buf[(*idx)++] = u.b[0];
}

static float get_float(const uint8_t *buf)
{
    union { float f; uint8_t b[4]; } u;
    u.b[3] = buf[0];
    u.b[2] = buf[1];
    u.b[1] = buf[2];
    u.b[0] = buf[3];
    return u.f;
}

static uint32_t protocol_time_now(void)
{
    return utc_base + (s - utc_base_tick);
}

static void protocol_time_set(uint32_t utc)
{
    utc_base = utc;
    utc_base_tick = s;
}

static void alarm_add(const char *ch, float limit, float value)
{
    char time_text[24];

    format_time(protocol_time_now(), time_text, sizeof(time_text));
    snprintf(alarm_records[alarm_head].text, sizeof(alarm_records[alarm_head].text), "%s | %s | %.2f | %.2f\r\n", time_text, ch, limit, value);
    alarm_head = (uint8_t)((alarm_head + 1U) % 10U);
    if(alarm_count < 10U) alarm_count++;
    alarm_save();
}

static void alarm_check(float ch0, float ch1, float ch2)
{
    uint8_t flags = 0U;

    if(ch0 >= proto_cfg.ch0_limit) flags |= 0x01U;
    if(ch1 >= proto_cfg.ch1_limit) flags |= 0x02U;
    if(ch2 >= proto_cfg.ch2_limit) flags |= 0x04U;

    if(((flags & 0x01U) != 0U) && ((baojing_biaoji & 0x01U) == 0U)) {
        alarm_add("CH0", proto_cfg.ch0_limit, ch0);
        if((proto_cfg.alarm_enable == 0x01U) && (proto_cfg.auto_report == 0U)) RS485_SendString(alarm_records[(alarm_head + 9U) % 10U].text);
    }
    if(((flags & 0x02U) != 0U) && ((baojing_biaoji & 0x02U) == 0U)) {
        alarm_add("CH1", proto_cfg.ch1_limit, ch1);
        if((proto_cfg.alarm_enable == 0x01U) && (proto_cfg.auto_report == 0U)) RS485_SendString(alarm_records[(alarm_head + 9U) % 10U].text);
    }
    if(((flags & 0x04U) != 0U) && ((baojing_biaoji & 0x04U) == 0U)) {
        alarm_add("CH2", proto_cfg.ch2_limit, ch2);
        if((proto_cfg.alarm_enable == 0x01U) && (proto_cfg.auto_report == 0U)) RS485_SendString(alarm_records[(alarm_head + 9U) % 10U].text);
    }
    baojing_biaoji = flags;
}

static void alarm_query(void)
{
    uint8_t i;
    uint8_t index;

    if(alarm_count == 0U) {
        RS485_SendString("empty");
        return;
    }
    for(i = 0U; i < alarm_count; i++) {
        index = (uint8_t)((alarm_head + 9U - i) % 10U);
        RS485_SendString(alarm_records[index].text);
        if(i != (alarm_count - 1U)) RS485_SendString("\r\n");
    }
}

static void alarm_clear(void)
{
    alarm_count = 0U;
    alarm_head = 0U;
    memset(alarm_records, 0, sizeof(alarm_records));
    alarm_save();
}

static void protocol_send(uint16_t id, uint8_t type, uint16_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[160];
    uint16_t idx = 0U;
    uint16_t crc;

    if(len > 140U) return;

    put_u16(frame, &idx, PROTO_START);
    put_u16(frame, &idx, id);
    frame[idx++] = type;
    put_u16(frame, &idx, cmd);
    frame[idx++] = len;
    frame[idx++] = PROTO_VERSION;
    if((payload != 0) && (len > 0U)) {
        memcpy(&frame[idx], payload, len);
        idx += len;
    }
    crc = crc16_modbus(frame, idx);
    put_u16(frame, &idx, crc);
    put_u16(frame, &idx, PROTO_END);
    bytes_to_ascii_send(frame, idx);
}

static void protocol_ok(uint16_t id, uint16_t cmd)
{
    uint8_t ok = 0xFFU;
    protocol_send(id, PROTO_TYPE_RESP, cmd, &ok, 1U);
}

static void protocol_error(uint16_t id)
{
    protocol_send(id, PROTO_TYPE_ERROR, CMD_ERROR, 0, 0U);
}

static void Zidong_Shangbao(uint16_t resp_id)
{
    uint8_t resp[20];
    uint16_t idx = 0U;
    float ch0;
    float ch1;
    float ch2;

    ch0 = App_Du_CH0(proto_cfg.ch0_ratio);
    ch1 = App_Du_CH1(proto_cfg.ch1_ratio);
    ch2 = App_Du_CH2();
    alarm_check(ch0, ch1, ch2);
    put_u32(resp, &idx, protocol_time_now());
    put_float(resp, &idx, ch0);
    put_float(resp, &idx, ch1);
    protocol_send(resp_id, PROTO_TYPE_RESP, CMD_AUTO_START, resp, (uint8_t)idx);
}

static uint32_t baud_from_code(uint8_t code)
{
    switch(code) {
    case 0x11U: return 4800U;
    case 0x12U: return 9600U;
    case 0x14U: return 115200U;
    default: return 19200U;
    }
}

static uint32_t interval_from_code(uint8_t code)
{
    switch(code) {
    case 0x02U: return 3U;
    case 0x03U: return 5U;
    default: return 1U;
    }
}

void Xieyi_Chushihua(void)
{
    protocol_defaults();
    Canshu_Duqu();
    alarm_load();
    App_Set_Dac(proto_cfg.dac_value);
    RS485_Config(proto_cfg.baudrate);
}

void Xieyi_Xintiao(void)
{
    protocol_send(proto_cfg.device_id, PROTO_TYPE_HEART, CMD_HEARTBEAT, 0, 0U);
}

static void handle_command(uint16_t frame_id, uint8_t type, uint16_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t resp[32];
    uint16_t idx = 0U;
    uint16_t resp_id = proto_cfg.device_id;

    if((frame_id != proto_cfg.device_id) && (frame_id != PROTO_BROADCAST_ID)) return;
    if((type != PROTO_TYPE_CMD) && (type != PROTO_TYPE_HEART)) {
        protocol_error(resp_id);
        return;
    }
    if((proto_cfg.auto_report != 0U) && (cmd != CMD_AUTO_STOP)) return;

    switch(cmd) {
    case CMD_FIND:
        Xieyi_Xintiao();
        break;
    case CMD_REBOOT:
        protocol_ok(resp_id, cmd);
        NVIC_SystemReset();
        break;
    case CMD_VERSION:
        resp[0] = 0x02U; resp[1] = 0x00U; resp[2] = 0x01U; resp[3] = 0x00U;
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, 4U);
        break;
    case CMD_GET_TIME:
        idx = 0U; put_u32(resp, &idx, protocol_time_now());
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_SET_TIME:
        if(len == 4U) { protocol_time_set(get_u32(payload)); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_GET_ID:
        idx = 0U; put_u16(resp, &idx, proto_cfg.device_id);
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_SET_ID:
        if(len == 2U) {
            proto_cfg.device_id = get_u16(payload);
            if((proto_cfg.device_id == 0U) || (proto_cfg.device_id == 0xFFFFU)) proto_cfg.device_id = PROTO_DEVICE_ID_DEFAULT;
            Canshu_Baocun();
            protocol_ok(proto_cfg.device_id, cmd);
        } else protocol_error(resp_id);
        break;
    case CMD_GET_BAUD:
        resp[0] = proto_cfg.baud_code;
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, 1U);
        break;
    case CMD_SET_BAUD:
        if(len == 1U) {
            proto_cfg.baud_code = payload[0];
            proto_cfg.baudrate = baud_from_code(proto_cfg.baud_code);
            Canshu_Baocun();
            protocol_ok(resp_id, cmd);
            delay_1ms(20U);
            RS485_SetBaudrate(proto_cfg.baudrate);
        } else protocol_error(resp_id);
        break;
    case CMD_CH0:
        idx = 0U; put_float(resp, &idx, App_Du_CH0(proto_cfg.ch0_ratio));
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_CH1:
        idx = 0U; put_float(resp, &idx, App_Du_CH1(proto_cfg.ch1_ratio));
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_CH2:
        idx = 0U; put_float(resp, &idx, App_Du_CH2());
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_SET_RATIO0:
        if(len == 4U) { proto_cfg.ch0_ratio = get_float(payload); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_SET_RATIO1:
        if(len == 4U) { proto_cfg.ch1_ratio = get_float(payload); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_SET_INTERVAL:
        if(len == 1U) { proto_cfg.interval_code = payload[0]; proto_cfg.interval_s = interval_from_code(proto_cfg.interval_code); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_SET_DAC:
        if(len == 2U) { proto_cfg.dac_value = get_u16(payload) & 0x0FFFU; App_Set_Dac(proto_cfg.dac_value); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_AUTO_START:
        proto_cfg.auto_report = 1U;
        proto_cfg.last_report_s = s;
        App_Xianshi_Zhuangtai("AutoSample");
        Zidong_Shangbao(resp_id);
        break;
    case CMD_AUTO_STOP:
        proto_cfg.auto_report = 0U;
        App_Xianshi_Zhuangtai("IDLE");
        protocol_ok(resp_id, cmd);
        break;
    case CMD_SLEEP:
        protocol_ok(resp_id, cmd);
        App_Shui10s(proto_cfg.baudrate);
        RS485_SendString("instrument wakeup");
        break;
    case CMD_GET_LIMIT_ALL:
        idx = 0U; put_float(resp, &idx, proto_cfg.ch0_limit); put_float(resp, &idx, proto_cfg.ch1_limit);
        protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_GET_LIMIT0:
        idx = 0U; put_float(resp, &idx, proto_cfg.ch0_limit); protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_GET_LIMIT1:
        idx = 0U; put_float(resp, &idx, proto_cfg.ch1_limit); protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_GET_LIMIT2:
        idx = 0U; put_float(resp, &idx, proto_cfg.ch2_limit); protocol_send(resp_id, PROTO_TYPE_RESP, cmd, resp, (uint8_t)idx);
        break;
    case CMD_SET_LIMIT0:
        if(len == 4U) { proto_cfg.ch0_limit = get_float(payload); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_SET_LIMIT1:
        if(len == 4U) { proto_cfg.ch1_limit = get_float(payload); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_SET_LIMIT2:
        if(len == 4U) { proto_cfg.ch2_limit = get_float(payload); Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_UPGRADE_REQ:
    {
        BootFlagTypeDef boot_flag;
        boot_flag.magic = PROTO_BOOT_FLAG_MAGIC;
        boot_flag.device_id = proto_cfg.device_id;
        boot_flag.baud_code = proto_cfg.baud_code;
        boot_flag.baudrate = proto_cfg.baudrate;
        protocol_ok(resp_id, cmd);
        flash_erase_range(PROTO_BOOT_FLAG_ADDR, 4096U);
        flash_write_bytes(PROTO_BOOT_FLAG_ADDR, (const uint8_t *)&boot_flag, sizeof(boot_flag));
        delay_1ms(50U);
        NVIC_SystemReset();
        break;
    }
    case CMD_UPGRADE_PREP:
        rs485_raw_mode = 1U;
        rs485_raw_len = 0U;
        rs485_raw_done = 0U;
        rs485_raw_last_s = s;
        upgrade_ready = 0U;
        upgrade_size = 0U;
        break;
    case CMD_UPGRADE_EXEC:
        if(upgrade_execute() != 0U) {
            protocol_ok(resp_id, cmd);
            delay_1ms(50U);
            NVIC_SystemReset();
        } else {
            protocol_error(resp_id);
        }
        break;
    case CMD_ALARM_ENABLE:
        if(len == 1U) { proto_cfg.alarm_enable = payload[0]; Canshu_Baocun(); protocol_ok(resp_id, cmd); }
        else protocol_error(resp_id);
        break;
    case CMD_ALARM_QUERY:
        alarm_query();
        break;
    case CMD_ALARM_CLEAR:
        alarm_clear();
        protocol_ok(resp_id, cmd);
        break;
    default:
        protocol_error(resp_id);
        break;
    }
}

static void process_frame(const uint8_t *frame, uint16_t len)
{
    uint16_t frame_id;
    uint8_t type;
    uint16_t cmd;
    uint8_t payload_len;
    uint16_t crc_recv;
    uint16_t crc_calc;
    uint16_t end;

    if(len < 13U) { protocol_error(proto_cfg.device_id); return; }
    if(get_u16(&frame[0]) != PROTO_START) { protocol_error(proto_cfg.device_id); return; }
    frame_id = get_u16(&frame[2]);
    type = frame[4];
    cmd = get_u16(&frame[5]);
    payload_len = frame[7];
    if(frame[8] != PROTO_VERSION) { protocol_error(proto_cfg.device_id); return; }
    if(len != (uint16_t)(13U + payload_len)) { protocol_error(proto_cfg.device_id); return; }
    crc_recv = get_u16(&frame[9U + payload_len]);
    crc_calc = crc16_modbus(frame, (uint16_t)(9U + payload_len));
    end = get_u16(&frame[11U + payload_len]);
    if(end != PROTO_END) { protocol_error(proto_cfg.device_id); return; }
    if(crc_recv != crc_calc) { protocol_error(proto_cfg.device_id); return; }
    handle_command(frame_id, type, cmd, &frame[9], payload_len);
}

void Xieyi_Chuli(void)
{
    uint8_t frame[128];
    uint16_t len;
    uint8_t ascii[RS485_RX_BUF_LEN];
    uint16_t ascii_len;

    App_Led_Chuli(proto_cfg.auto_report);

    if(rs485_raw_mode != 0U) {
        if((rs485_raw_done != 0U) || ((rs485_raw_len > 0U) && ((s - rs485_raw_last_s) >= 1U))) {
            __disable_irq();
            rs485_raw_mode = 0U;
            rs485_raw_done = 1U;
            __enable_irq();
            if(upgrade_store() != 0U) protocol_ok(proto_cfg.device_id, CMD_UPGRADE_PREP);
            else protocol_error(proto_cfg.device_id);
        }
        return;
    }

    if(rs485_rx_done != 0U) {
        __disable_irq();
        ascii_len = (uint16_t)strlen((const char *)rs485_rx_buf);
        memcpy(ascii, (const void *)rs485_rx_buf, ascii_len + 1U);
        rs485_rx_done = 0U;
        __enable_irq();
        if(ascii_to_bytes(ascii, ascii_len, frame, sizeof(frame), &len) != 0U) process_frame(frame, len);
        else protocol_error(proto_cfg.device_id);
    }

    if(proto_cfg.auto_report != 0U) {
        if((s - proto_cfg.last_report_s) >= proto_cfg.interval_s) {
            proto_cfg.last_report_s = s;
            Zidong_Shangbao(proto_cfg.device_id);
        }
    }
}
