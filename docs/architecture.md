# 系统架构说明

## 1. 硬件平台

| 项目 | 配置 |
| --- | --- |
| MCU | GD32F470VET6（Cortex-M4F，512KB Flash，192KB RAM） |
| 主频 | 240MHz（HXTAL 25MHz + PLL，`system_gd32f4xx.c` 选中 `__SYSTEM_CLOCK_240M_PLL_25M_HXTAL`） |
| 总线时钟 | AHB 240MHz，APB1 60MHz，APB2 120MHz；APB1 定时器时钟 120MHz，APB2 定时器时钟 240MHz |
| 调试/下载 | SWD（Keil + J-Link / CMSIS-DAP） |

### 外设与引脚

| 功能 | 资源 | 引脚 / 参数 |
| --- | --- | --- |
| RS485 | USART1，中断收 | PA2 TX / PA3 RX（AF7），PA1 为 DE/RE 方向控制，默认 19200 8N1 |
| OLED | 软件 IIC（SSD1306，128×32） | PB4 SCL / PB5 SDA，开漏，从机地址 0x78 |
| PT100 采集 | AD3344（SPI0） | PA5 SCK / PA6 MISO / PA7 MOSI（AF5），CS = PB6，CPOL=0/CPHA=1，100SPS，PGA 4.096V |
| 外部 SPI Flash | SPI1（GD25Q 系列） | PB13 SCK / PB14 MISO / PB15 MOSI（AF5），CS = PB12，模式 0 |
| 模拟输入 CH0 | ADC0 规则通道 0 | PA0，采样时间 480 周期 |
| 模拟输出 CH1 | DAC0_OUT0 | PA4，`Analog_ReadCH1()` 返回设定值回读 |
| 状态灯 | GPIO 推挽输出 | LED1 = PE3（心跳，1Hz 翻转），LED2 = PE5（自动上报指示） |
| RTC | LXTAL，失败回落 IRC32K | 自动唤醒 1Hz（EXTI_22），用于 10s 深度睡眠 |

> AD3344 与外部 SPI Flash 分别挂在 SPI0 / SPI1 上，CS 各自独立，不存在片选冲突。

## 2. 软件分层

```
APP/User          main.c / gd32f4xx_it.c / systick.c         ← 入口、中断向量、1ms 延时
APP/Function      Function.c / fun.c / AppService.c          ← 上电流程、主循环、业务服务
APP/Protocol      Protocol.c                                 ← 帧收发、命令分发、参数/报警/IAP
APP/Driver/...    RS485 / OLED / IIC / RTC / Timer / LED /
                  Flash / ADC(Analog+PT100) / System         ← 设备驱动
APP/Library       GigaDevice GD32F4xx 标准外设库
APP/CMSIS         ARM CMSIS + 厂商系统文件与启动文件
Bootloader/       同构的独立工程（UART + OLED + 片内 Flash + 协议子集）
```

数据流：

```
上位机 ──RS485帧──▶ USART1_IRQHandler(收字节，识别帧尾) ──▶ Xieyi_Chuli()
                                                            │  ASCII十六进制解码
                                                            ▼
                                                    process_frame() 校验
                                                            │
                                                            ▼
                                                    handle_command() 分发
                       ┌────────────────────┬───────────────┴───────────────┐
                       ▼                    ▼                               ▼
                采集(CH0/CH1/CH2)      参数/报警(SPI Flash)            升级(片内 Flash)
                       │                    │                               │
                       └──────── protocol_send() 组帧 ──▶ RS485 发送 ◀──────┘
```

## 3. Flash / RAM 分区

### 片内 Flash（512KB）

| 区间 | 大小 | 用途 |
| --- | --- | --- |
| `0x08000000` ~ `0x08010000` | 64KB | Bootloader（`gd32f470ve_boot.ld`） |
| `0x08010000` ~ `0x08011000` | 4KB | 升级标志页（魔术字 `0x424F4F54` "BOOT" + 设备地址 + 波特率），由 Bootloader 读取后立即擦除 |
| `0x08011000` ~ `0x08031000` | 128KB | APP 运行区（`gd32f470ve_app.ld`，`APP_IMAGE` 宏使 `SCB->VTOR` 指向此地址） |
| `0x08031000` ~ `0x08051000` | 128KB | 备份区（预留，当前代码未写入） |
| `0x08051000` ~ `0x08071000` | 128KB | 升级暂存区：APP 把收到的镜像写到这里，Bootloader 校验后搬到运行区 |

### 外部 SPI Flash

| 偏移 | 用途 |
| --- | --- |
| `0x000000` | 应用参数（魔术字 `0x50415241` "PARA"）：设备地址、波特率档位、CH0/CH1 标定系数、三路阈值、上报周期、DAC 值、报警使能 |
| `0x001000` | 报警记录环形缓冲（魔术字 `0x414C4D31` "ALM1"，最多 10 条 × 80 字节文本） |

### RAM

| 区域 | 大小 | 说明 |
| --- | --- | --- |
| `rs485_raw_buf` | 128KB | 升级镜像接收缓冲（`.bss`，占用 ZI 的大头） |
| OLED 显存 | 512B | `128 × 4` 字节 |
| 参数 / 报警缓存 | ~1KB | `ProtocolConfigTypeDef` + 10 条记录 |
| 栈 + 堆 | 2KB + 2KB | 由 `.ld` 的 `_user_heap_stack` 预留 |

## 4. 时序与中断

| 时基 | 来源 | 用途 |
| --- | --- | --- |
| 1ms | SysTick（`systick_config()`） | `delay_1ms()` 忙等延时 |
| 1s | TIMER2 更新中断（`Tim_Init()`） | 全局秒计数 `volatile uint32_t s`，LED 心跳、自动上报周期、原始模式空闲判定、UTC 推算 |

* `s` 由中断写、主循环读，**必须** `volatile`；所有差值判断都用无符号相减（回绕安全）。
* TIMER2 的 prescaler/period 由 `SystemCoreClock` 推导：240MHz 下得到 23999 / 4999，与硬件原值一致；
  换主频后仍能保持 1 秒节拍。
* 中断优先级：USART1 (0,0)、TIMER2 (0,0)、SysTick 0、RTC 唤醒 (1,1)。
* 关键共享变量（`rs485_rx_done`、`rs485_raw_*`）在读写时用 `__disable_irq()/__enable_irq()` 做短临界区；
  十六进制解码直接在接收缓冲上完成（临界区约 3µs，远小于一个字节的传输时间），不再额外拷贝 256 字节到栈上。

### 典型耗时

| 操作 | 时间 |
| --- | --- |
| CH2（PT100）单次读取 | 4 次 AD3344 转换 × 15ms ≈ 60ms（100SPS 档） |
| OLED 整屏刷新 | 512 字节数据 + 命令，软件 IIC 约 20ms |
| 一帧协议处理 | 帧长 ≤ 34 字节，编解码 + CRC 约 0.1ms |
| 升级镜像写入 | 128KB，按 32 位字编程 ≈ 3.3 万次编程操作（原先逐字节为 13 万次） |

## 5. 升级流程（IAP）

```
上位机                          APP                          Bootloader
  │ 0x0303 AUTO_STOP ──────────▶ 停止上报
  │ 0x0502 UPGRADE_PREP ───────▶ 进入原始模式（挂起应答）
  │ [magic 0x5AA5C33C + 镜像] ─▶ 收到 128KB 大缓冲
  │                              1s 空闲 → 擦暂存区 + 写入
  │ ◀─────────── ok(0x0502) ────┘
  │ 0x0503 UPGRADE_EXEC ───────▶ 写升级标志页 → 软复位
  │                                                        │ 读到标志 → 擦标志页 → 停留在 Bootloader
  │ ◀──── "using command to interrupt start Application" ──┤
  │ 0x0502 UPGRADE_PREP ──────────────────────────────────▶ 进入原始模式
  │ [magic + 镜像] ───────────────────────────────────────▶ 500ms 空闲 → 擦暂存区 + 写入
  │ ◀──────────────────────────────── ok(0x0502) ─────────┤
  │ 0x0503 UPGRADE_EXEC ──────────────────────────────────▶ 校验 SP/PC → 擦运行区 → 暂存区搬运行区
  │ ◀──────────────────────────────── ok(0x0503) ─────────┤ → 软复位 → 运行新 APP
```

要点：

* APP 与 Bootloader 使用**同一个原始模式协议**（魔术字 `0x5AA5C33C` + 镜像字节流），
  区别在于 APP 的空闲判定为 1s、Bootloader 为 500ms。
* Bootloader 在搬运前会校验镜像的栈顶（`0x20000000` ~ `0x20030000`）与复位向量
  （落在 `0x08011000` ~ `0x08031000` 且 bit0 为 1），校验不通过直接拒绝，不会写坏运行区。
* 升级标志页只占 4KB，Bootloader 读到后立刻擦除，避免断电后反复进入升级模式。
* 写入统一走“32 位字编程 + 收尾字节编程 + 回读校验”，失败即返回错误帧并保留原运行区。

## 6. 低功耗

`0x03AA` 命令触发 `App_Shui10s()`：配置 RTC 自动唤醒（1Hz，计数值 9 = 10 个周期），
关掉 USART 接收中断、TIMER2 与 SysTick 后执行 `WFI` 进入深度睡眠；
被 RTC 唤醒后重新 `SystemInit()`、重配 SysTick / RS485 / TIMER2，并把秒计数补 10 秒。

> RTC 唤醒走的是 `RTC_WKUP_IRQn`（IRQ3），**必须**在启动文件的向量表里挂上
> `RTC_WKUP_IRQHandler`。Keil 用的厂商启动文件里有；仓库里的 GCC 启动文件原先没有
> （落在 `.rept` 填充区指向 `Default_Handler`），唤醒后会跳进死循环，现已按厂商顺序
> 重新生成完整向量表。

## 7. 构建产物与配置

| 工程 | Keil 工程 | GCC 链接脚本 | 起始地址 | 大小 |
| --- | --- | --- | --- | --- |
| APP | `APP/project/CIMC_GD32_Template.uvprojx` | `APP/project/gd32f470ve_app.ld` | 0x08011000 | 128KB |
| Bootloader | `Bootloader/project/CIMC_BOOT.uvprojx` | `Bootloader/project/gd32f470ve_boot.ld` | 0x08000000 | 64KB |

* Keil 编译宏：`USE_STDPERIPH_DRIVER, GD32F470`（APP 另加 `APP_IMAGE`）。
* GCC 编译宏相同，见根目录 `Makefile`；`OPT=-O2` 可调，「`make size`」输出各段大小。
* 两个工程共用同一份协议定义约定（帧格式完全一致），但代码各自独立编译，互不引用。
