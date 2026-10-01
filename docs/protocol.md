# RS485 通信协议

APP（`APP/Protocol/Protocol.c`）与 Bootloader（`Bootloader/Protocol/Boot_Protocol.c`）使用**同一套帧格式**，
Bootloader 只实现其中与升级相关的命令。物理层为 RS485 半双工，USART1，默认 **19200 8N1**，
方向由 PA1（DE/RE）控制，收发切换由软件在发送前后完成。

---

## 1. 帧格式

逻辑帧是定长头 + 变长参数 + CRC + 帧尾，**在线上以大写十六进制文本传输**
（每 1 个字节编码为 2 个 ASCII 字符，例如 `0xA5 0xB6` 发送为字符 `A5B6`）。

| 偏移 | 长度 | 字段 | 说明 |
| --- | --- | --- | --- |
| 0 | 2 | START | 固定 `0xA5B6`（大端） |
| 2 | 2 | ID | 设备地址，`0xFFFF` 为广播 |
| 4 | 1 | TYPE | `0x01` 命令 / `0x02` 应答 / `0x05` 心跳 / `0xFF` 错误 |
| 5 | 2 | CMD | 命令码，大端 |
| 7 | 1 | LEN | 参数长度，0 ~ 140 |
| 8 | 1 | VERSION | 协议版本，当前 `0x02` |
| 9 | LEN | PAYLOAD | 参数 |
| 9+LEN | 2 | CRC | CRC16-Modbus，大端 |
| 11+LEN | 2 | END | 固定 `0xB6A5`（大端） |

* 帧总长 = `13 + LEN` 字节（未编码）；编码后字符数 = `2 × (13 + LEN)`。
* **CRC16-Modbus**：初值 `0xFFFF`，多项式 `0xA001`（反向），计算范围是帧的 `[0, 9+LEN)` 字节（不含 CRC 与 END）。
* 多字节整数（u16/u32）与 IEEE-754 浮点均为**大端**。
* 接收侧同时支持两种结束条件：收到 `\r` / `\n`，或收到文本 `B6A5`（帧尾的十六进制文本）。
  因此上位机发送时**不要**在帧文本里换行会更稳妥，两种方式都能解析。

### 应答约定

* 成功：TYPE=`0x02`，CMD 与请求相同，PAYLOAD 为 1 字节 `0xFF`（无返回数据的命令）。
* 失败：TYPE=`0xFF`，CMD=`0xEEEE`，无 PAYLOAD。
* 校验错、长度错、版本错、地址不匹配之外的非法帧都会回错误帧。
* ID 不是本机地址且不是广播 `0xFFFF` 的帧会被**直接丢弃**（不应答）。
* 自动上报（`0x0302`）开启期间，除 `0x0303` 外的命令一律忽略。

---

## 2. 命令表

### 系统类

| 命令码 | 名称 | 请求参数 | 应答 |
| --- | --- | --- | --- |
| `0xFFFF` | FIND 查找设备 | 无 | 心跳帧（TYPE=`0x05`，CMD=`0x8888`） |
| `0x0101` | REBOOT 重启 | 无 | `0xFF`，随后软复位 |
| `0x0104` | VERSION 版本 | 无 | 4 字节 `02 00 01 00` |
| `0x0105` | SET_TIME 设置时间 | 4 字节 u32（UTC 秒） | `0xFF` |
| `0x0106` | GET_TIME 读时间 | 无 | 4 字节 u32（UTC 秒，上电基准 + 系统秒计数） |
| `0x0111` | GET_ID 读地址 | 无 | 2 字节 u16 |
| `0x01A1` | SET_ID 设地址 | 2 字节 u16（`0x0000`/`0xFFFF` 视为非法，回落默认 `0x0008`） | `0xFF`，以新地址应答 |
| `0x0112` | GET_BAUD 读波特率 | 无 | 1 字节波特率档位 |
| `0x01A2` | SET_BAUD 设波特率 | 1 字节档位（`0x11`=4800 `0x12`=9600 `0x13`=19200 `0x14`=115200，其它值按 19200） | 先以旧波特率应答，20ms 后切换 |
| `0x03AA` | SLEEP 休眠 | 无 | `0xFF`，随后深度睡眠约 10s，唤醒后回 `instrument wakeup` |

### 采集类

| 命令码 | 名称 | 请求参数 | 应答 |
| --- | --- | --- | --- |
| `0x0201` | CH0 电压 | 无 | float（片内 ADC / PA0 × 标定系数） |
| `0x0202` | CH1 电压 | 无 | float（DAC 设定值回读 × 标定系数） |
| `0x0221` | CH2 温度 | 无 | float（PT100 温度，℃） |
| `0x0241` | SET_RATIO0 | 4 字节 float | `0xFF` |
| `0x0242` | SET_RATIO1 | 4 字节 float | `0xFF` |
| `0x0261` | SET_INTERVAL 上报周期 | 1 字节档位（`0x01`=1s `0x02`=3s `0x03`=5s，其它按 1s） | `0xFF` |
| `0x0302` | AUTO_START 开始自动上报 | 无 | 立即上报一帧（见下），此后按周期上报 |
| `0x0303` | AUTO_STOP 停止自动上报 | 无 | `0xFF` |

自动上报帧：TYPE=`0x02`，CMD=`0x0302`，PAYLOAD = 4 字节 UTC + float CH0 + float CH1（共 12 字节）；
同时按阈值检查 CH0/CH1/CH2 是否越限并记录报警。上报期间 LED2 常亮。

### DAC 与阈值

| 命令码 | 名称 | 请求参数 | 应答 |
| --- | --- | --- | --- |
| `0x0301` | SET_DAC | 2 字节 u16（与 `0x0FFF` 掩码） | `0xFF` |
| `0x0400` | GET_LIMIT_ALL | 无 | float CH0 阈值 + float CH1 阈值 |
| `0x0401`/`0x0402`/`0x0403` | GET_LIMIT0/1/2 | 无 | float |
| `0x0411`/`0x0412`/`0x0413` | SET_LIMIT0/1/2 | 4 字节 float | `0xFF` |

默认阈值：CH0 = 3.0V，CH1 = 3.0V，CH2 = 150.0℃。

### 报警

| 命令码 | 名称 | 请求参数 | 应答 |
| --- | --- | --- | --- |
| `0x0601` | ALARM_ENABLE | 1 字节（1 使能，0 关闭） | `0xFF` |
| `0x0602` | ALARM_QUERY | 无 | **非帧格式**：直接回最多 10 条文本记录，无记录时回 `empty` |
| `0x0603` | ALARM_CLEAR | 无 | `0xFF` |

报警记录格式（每条一行）：

```
2026-06-07 12:34:56 | CH0 | 3.00 | 3.42\r\n
```

记录保存在外部 SPI Flash 的 `0x001000`（环形缓冲，最多 10 条，掉电不丢）。
越限只在“由正常变为越限”的瞬间记录一次（回差标志位 `baojing_biaoji`），
报警使能且**未开启自动上报**时才主动推送该条记录。

### 升级（IAP）

| 命令码 | 名称 | 请求参数 | 应答 |
| --- | --- | --- | --- |
| `0x0501` | UPGRADE_REQ | 无 | `0xFF`，写升级标志后重启进入 Bootloader |
| `0x0502` | UPGRADE_PREP | 无（APP）/ 无（BOOT，进入原始模式） | 收到完整镜像并写入暂存区后回 `0xFF` |
| `0x0503` | UPGRADE_EXEC | 无 | `0xFF`，随后搬运镜像并复位 |

升级流程详见 [architecture.md](architecture.md#升级流程-iap)。

---

## 3. 参数持久化

应用参数（设备地址、波特率、标定系数、阈值、上报周期、DAC 值、报警使能）保存在
外部 SPI Flash 的 `0x000000`，带魔术字 `0x50415241`（"PARA"）；写入前先做 4KB 扇区擦除，
读取时校验魔术字与地址合法性，非法则使用默认值。

> 注意：自动上报状态与“上次上报时刻”**不**持久化，上电一律为停止状态。

## 4. 上位机示例

设置设备地址为 0x0008 并开启 CH0 自动上报（十六进制文本，含计算好的 CRC）：

```python
import struct

def build(dev_id, cmd, payload=b"", version=0x02):
    body = struct.pack(">HHBHB", 0xA5B6, dev_id, 0x01, cmd, len(payload)) + bytes([version]) + payload
    crc = 0xFFFF
    for b in body:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    frame = body + struct.pack(">HH", crc, 0xB6A5)
    return frame.hex().upper().encode()          # 线上传输的是 ASCII 文本

print(build(0x0008, 0x0302))                     # AUTO_START
print(build(0x0008, 0x01A1, struct.pack(">H", 8)))  # SET_ID = 8
```
