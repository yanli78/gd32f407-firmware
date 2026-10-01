#include "Boot_Protocol.h"
#include "Boot_Flash.h"
#include "Boot_RS485.h"
#include "Boot_Timer.h"

static uint8_t upgrade_ready = 0U;
static uint32_t upgrade_size = 0U;
static uint8_t boot_stay_requested = 0U;
static uint16_t boot_device_id = BOOT_DEVICE_ID;

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
    return val < 10U ? (char)('0' + val) : (char)('A' + val - 10U);
}

static uint16_t crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint8_t j;
    for(i = 0U; i < len; i++) {
        crc ^= data[i];
        for(j = 0U; j < 8U; j++) crc = (crc & 1U) ? (uint16_t)((crc >> 1U) ^ 0xA001U) : (uint16_t)(crc >> 1U);
    }
    return crc;
}

static uint16_t get_u16(const uint8_t *b)
{
    return ((uint16_t)b[0] << 8U) | b[1];
}

static uint32_t get_u32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24U) | ((uint32_t)b[1] << 16U) | ((uint32_t)b[2] << 8U) | b[3];
}

static void put_u16(uint8_t *b, uint16_t *idx, uint16_t v)
{
    b[(*idx)++] = (uint8_t)(v >> 8U); b[(*idx)++] = (uint8_t)v;
}

static uint8_t ascii_to_bytes(const uint8_t *ascii, uint16_t ascii_len, uint8_t *out, uint16_t *out_len)
{
    uint16_t i;
    uint8_t h, l;
    if((ascii_len & 1U) != 0U) return 0U;
    *out_len = ascii_len / 2U;
    for(i = 0U; i < *out_len; i++) {
        h = hex_value(ascii[i * 2U]); l = hex_value(ascii[i * 2U + 1U]);
        if((h > 0x0FU) || (l > 0x0FU)) return 0U;
        out[i] = (uint8_t)((h << 4U) | l);
    }
    return 1U;
}

static void send_frame(uint16_t id, uint8_t type, uint16_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[64];
    char out[128];
    uint16_t idx = 0U;
    uint16_t i, crc;
    put_u16(frame, &idx, 0xA5B6U); put_u16(frame, &idx, id); frame[idx++] = type; put_u16(frame, &idx, cmd); frame[idx++] = len; frame[idx++] = BOOT_VERSION;
    for(i = 0U; i < len; i++) frame[idx++] = payload[i];
    crc = crc16(frame, idx); put_u16(frame, &idx, crc); put_u16(frame, &idx, 0xB6A5U);
    for(i = 0U; i < idx; i++) { out[i * 2U] = hex_char(frame[i] >> 4U); out[i * 2U + 1U] = hex_char(frame[i]); }
    out[idx * 2U] = '\0';
    Boot_RS485_SendString(out);
}

static void send_ok(uint16_t cmd)
{
    uint8_t ok = 0xFFU;
    send_frame(boot_device_id, 0x02U, cmd, &ok, 1U);
}

static void send_error(void)
{
    send_frame(boot_device_id, 0xFFU, 0xEEEEU, 0, 0U);
}

static void send_cmd_error(uint16_t cmd)
{
    send_frame(boot_device_id, 0xFFU, cmd, 0, 0U);
}

static uint8_t store_upgrade(void)
{
    const volatile uint8_t *raw_buf = Boot_RS485_RawBuf();
    uint32_t raw_len = Boot_RS485_RawLen();
    if(raw_len <= 4U) { upgrade_ready = 0U; upgrade_size = 0U; return 0U; }
    if(get_u32((const uint8_t *)raw_buf) != BOOT_UPGRADE_MAGIC) { upgrade_ready = 0U; upgrade_size = 0U; return 0U; }
    upgrade_size = raw_len - 4U;
    if(upgrade_size > BOOT_SLOT_SIZE) { upgrade_ready = 0U; upgrade_size = 0U; return 0U; }
    if(Boot_Flash_Erase(BOOT_TEMP_ADDR, BOOT_SLOT_SIZE) == 0U) { upgrade_ready = 0U; upgrade_size = 0U; return 0U; }
    if(Boot_Flash_Write(BOOT_TEMP_ADDR, (const uint8_t *)&raw_buf[4], upgrade_size) == 0U) { upgrade_ready = 0U; upgrade_size = 0U; return 0U; }
    upgrade_ready = 1U;
    return 1U;
}

uint8_t Boot_Protocol_ExecuteUpgrade(void)
{
    uint8_t buf[256];
    uint32_t offset = 0U;
    uint32_t chunk;
    if(upgrade_ready == 0U) return 0U;
    if(upgrade_size == 0U) return 0U;
    if((*(volatile uint32_t *)BOOT_TEMP_ADDR < 0x20000000U) || (*(volatile uint32_t *)BOOT_TEMP_ADDR > 0x20030000U)) return 0U;
    if((*(volatile uint32_t *)(BOOT_TEMP_ADDR + 4U) < BOOT_APP_ADDR) || (*(volatile uint32_t *)(BOOT_TEMP_ADDR + 4U) >= BOOT_APP_END) || ((*(volatile uint32_t *)(BOOT_TEMP_ADDR + 4U) & 1U) == 0U)) return 0U;
    if(Boot_Flash_Erase(BOOT_APP_ADDR, BOOT_SLOT_SIZE) == 0U) return 0U;
    while(offset < upgrade_size) {
        chunk = upgrade_size - offset;
        if(chunk > sizeof(buf)) chunk = sizeof(buf);
        memcpy(buf, (const void *)(BOOT_TEMP_ADDR + offset), chunk);
        if(Boot_Flash_Write(BOOT_APP_ADDR + offset, buf, chunk) == 0U) return 0U;
        offset += chunk;
    }
    return 1U;
}

static void handle_frame(const uint8_t *frame, uint16_t len)
{
    uint16_t id, cmd, crc_recv, crc_calc, end;
    uint8_t type, payload_len;
    if(len < 13U) { send_error(); return; }
    if(get_u16(&frame[0]) != 0xA5B6U) { send_error(); return; }
    id = get_u16(&frame[2]); type = frame[4]; cmd = get_u16(&frame[5]); payload_len = frame[7];
    if((id != boot_device_id) && (id != 0xFFFFU)) return;
    if(type != 0x01U && type != 0x05U) { send_error(); return; }
    if(frame[8] != BOOT_VERSION) { send_error(); return; }
    if(len != (uint16_t)(13U + payload_len)) { send_error(); return; }
    crc_recv = get_u16(&frame[9U + payload_len]); crc_calc = crc16(frame, (uint16_t)(9U + payload_len)); end = get_u16(&frame[11U + payload_len]);
    if((end != 0xB6A5U) || (crc_recv != crc_calc)) { send_error(); return; }
    if(cmd == 0xFFFFU) { send_frame(boot_device_id, 0x05U, 0x8888U, 0, 0U); return; }
    if(cmd == 0x0104U) { uint8_t v[4] = {0x02U,0x00U,0x01U,0x00U}; send_frame(boot_device_id, 0x02U, cmd, v, 4U); return; }
    if(cmd == 0x0501U) { boot_stay_requested = 1U; send_ok(cmd); Boot_RS485_SendString("\r\nusing command to interrupt start Application\r\nwait for start Application(10s)......\r\nwait for start Application(7s)......\r\nwait for start Application(4s)......\r\nwait for start Application(1s)......\r\n"); return; }
    if(cmd == 0x0502U) { boot_stay_requested = 1U; upgrade_ready = 0U; upgrade_size = 0U; Boot_RS485_StartRaw(); return; }
    if(cmd == 0x0503U) {
        if((upgrade_ready != 0U) && (upgrade_size != 0U)) {
            send_ok(cmd);
            Boot_DelayMs(100U);
            if(Boot_Protocol_ExecuteUpgrade() != 0U) {
                Boot_DelayMs(50U);
                NVIC_SystemReset();
            }
        } else send_error();
        return;
    }
    send_error();
}

void Boot_Protocol_Init(uint16_t device_id)
{
    boot_device_id = device_id;
}

void Boot_Protocol_Process(void)
{
    uint8_t frame[128];
    uint16_t len;
    uint8_t ascii[RS485_RX_BUF_LEN];
    uint16_t ascii_len;
    if(Boot_RS485_RawFinished() != 0U) {
        if(store_upgrade() != 0U) send_ok(0x0502U); else send_cmd_error(0x0502U);
        return;
    }
    if(Boot_RS485_GetFrame(ascii, &ascii_len) != 0U) {
        if(ascii_to_bytes(ascii, ascii_len, frame, &len) != 0U) handle_frame(frame, len); else send_error();
    }
}

uint8_t Boot_Protocol_Stay(void)
{
    return boot_stay_requested;
}

uint8_t Boot_Protocol_UpgradeReady(void)
{
    return upgrade_ready;
}

uint32_t Boot_Protocol_UpgradeSize(void)
{
    return upgrade_size;
}
