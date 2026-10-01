# GD32F470VE 固件工程（APP + Bootloader）

[![build](https://github.com/furuiking/gd32f407-firmware/actions/workflows/build.yml/badge.svg)](https://github.com/furuiking/gd32f407-firmware/actions/workflows/build.yml)

2025 CIMC IHD-V04 平台上的 GD32F470VET6 采集/通信板固件，包含**两个可直接烧录的独立工程**：

* **APP**（`APP/`，起始地址 `0x08011000`）——采集、RS485 协议、OLED 显示、参数与报警存储，支持在线升级；
* **Bootloader**（`Bootloader/`，起始地址 `0x08000000`）——上电校验并跳转 APP，接收升级镜像并搬运到运行区。

两者使用**同一套 RS485 帧协议**（帧格式见 [docs/protocol.md](docs/protocol.md)），但代码各自独立编译、互不引用。

## 功能

| 分类 | 内容 |
| --- | --- |
| 通信 | RS485（USART1，默认 19200 8N1），ASCII 十六进制帧 + CRC16-Modbus；支持设备寻址与广播；波特率可改（4800/9600/19200/115200） |
| 采集 | CH0：片内 ADC（PA0）× 标定系数；CH1：DAC 设定值回读；CH2：PT100 + AD3344（SPI0），实测电阻经标定表修正后按 Callendar-Van Dusen 反解温度 |
| 输出 | 12 位 DAC（0~4095）；LED1 每秒翻转（心跳）、LED2 指示自动上报 |
| 存储 | 外部 SPI Flash：应用参数（设备地址/波特率/标定系数/三路阈值/上报周期/DAC/报警使能）+ 10 条报警记录环形缓冲，掉电不丢 |
| 上报 | 自动上报周期 1/3/5 秒可设，可随时启停；越限报警按上升沿记录并可主动推送 |
| 低功耗 | 休眠命令进入深度睡眠约 10 秒，RTC 自动唤醒后自行恢复外设 |
| 升级 | IAP：APP 收包写暂存区，Bootloader 校验栈顶/复位向量后搬运到运行区 |
| 显示 | 128×32 SSD1306 OLED（软件 IIC），显示设备编号与工作状态 |

## 目录结构

```
.
├── APP/                          # 应用程序
│   ├── User/                     # main.c / 中断 / SysTick 延时 / newlib 桩
│   ├── Function/                 # 上电流程、主循环、业务服务（AppService）
│   ├── Protocol/                 # 帧收发、命令分发、参数/报警/IAP
│   ├── Driver/                   # UART(RS485)、OLED、IIC、RTC、Timer、LED、Flash、ADC、System
│   ├── Library/  CMSIS/          # GigaDevice 标准外设库与 CMSIS（厂商代码，保持原样）
│   ├── Startup/                  # GCC 启动文件（Keil 用 CMSIS 下的厂商启动文件）
│   └── project/                  # Keil 工程与 GCC 链接脚本
├── Bootloader/                   # Bootloader（结构与 APP 对称）
├── docs/                         # 协议文档、架构说明
├── Makefile                      # GNU Arm Embedded 构建
└── .github/workflows/build.yml   # CI：编译 APP + Bootloader 并上传产物
```

## 构建

### 方式一：Keil MDK（原工程方式）

* `APP/project/CIMC_GD32_Template.uvprojx`
* `Bootloader/project/CIMC_BOOT.uvprojx`

编译宏 `USE_STDPERIPH_DRIVER, GD32F470`（APP 另加 `APP_IMAGE`），器件包 `GigaDevice.GD32F4xx_DFP`。

### 方式二：GNU Arm Embedded Toolchain

```bash
sudo apt-get install gcc-arm-none-eabi make     # 或使用 xPack / Arm 官方工具链
make                # 编译 APP 与 Bootloader，输出 elf/hex/bin 到 build/
make app            # 只编译 APP
make boot           # 只编译 Bootloader
make size           # 查看各段大小
make clean
```

`CROSS=` 可指定工具链前缀，`OPT=` 可改优化等级（默认 `-O2`）。

已验证环境：**arm-none-eabi-gcc 15.2.1**（含 newlib-nano），APP 与 Bootloader 均为 **0 error / 0 warning**（`-Wall -Wextra`）。

| 镜像 | FLASH 占用 | RAM 占用 | 产物 |
| --- | --- | --- | --- |
| APP | 41012 B / 128 KB（31.3%） | 137720 B / 192 KB（70.1%） | `build/CIMC_GD32_Template.{elf,hex,bin}` |
| Bootloader | 9136 B / 64 KB（13.9%） | 134344 B / 192 KB（68.3%） | `build/CIMC_BOOT.{elf,hex,bin}` |

> 上表为 arm-none-eabi-gcc 15.2.1 的结果；CI 用的 Ubuntu 包（gcc 13.2）分别为
> APP 45460 B、Bootloader 8948 B，不同工具链版本间有几百字节到 4 KB 的正常差异。
> RAM 占用的大头是升级镜像接收缓冲（128 KB `.bss`）。
> 原来的 Keil AC5 工程在 `-O0` 下的参考值：APP Code 20250 + RO 6842 字节，Bootloader Code 7122 + RO 970 字节。

## 内存布局

| 区间 | 大小 | 用途 |
| --- | --- | --- |
| `0x08000000` ~ `0x08010000` | 64 KB | Bootloader |
| `0x08010000` ~ `0x08011000` | 4 KB | 升级标志页（Bootloader 读取后立即擦除） |
| `0x08011000` ~ `0x08031000` | 128 KB | APP 运行区 |
| `0x08031000` ~ `0x08051000` | 128 KB | 备份区（预留，当前未使用） |
| `0x08051000` ~ `0x08071000` | 128 KB | 升级暂存区 |

外设引脚、时序与中断设计、IAP 完整时序见 [docs/architecture.md](docs/architecture.md)。

## 通信协议

逻辑帧：`START(A5B6) | ID(2) | TYPE(1) | CMD(2) | LEN(1) | VER(1) | PAYLOAD | CRC16(2) | END(B6A5)`，
在线上以**大写十六进制文本**传输；帧总长 13 + LEN 字节。完整命令表、CRC 算法与上位机示例见
[docs/protocol.md](docs/protocol.md)。

```python
# 读取 CH2 温度（设备地址 0x0008）
frame = build(0x0008, 0x0221)   # 见 docs/protocol.md 中的 build()
```

## 本次优化内容

在**不改动协议帧格式、不改变外设时序与采样数值**的前提下做了如下整理
（逐条见 `git log`，每个提交都有说明）：

### 缺陷修复

| 问题 | 位置 | 处理 |
| --- | --- | --- |
| RTC 唤醒中断未挂到向量表，深度睡眠唤醒后跳进 `Default_Handler` 死循环 | `Startup/startup_gd32f470_gcc.s` | 按厂商顺序重新生成 107 项完整向量表（GCC 构建；Keil 用的是厂商启动文件） |
| 片内 Flash 编程前未清 FMC 错误标志，残留标志会让写入直接失败 | `Protocol.c`、`Boot_Flash.c` | 擦写前清标志；擦除增加页对齐校验 |
| 帧文本超过 126 字节会被静默丢弃 | `Protocol.c` | 十六进制编码改分块发送 |
| `oled_show_char` 传入非可显示字符会越界读字库 | `OLED.c` | 增加 ASCII 范围检查 |
| ADC 转换标志轮询没有超时，硬件异常会死等 | `Analog.c` | 加超时退出 |
| `spi_flash_buffer_erase()` 按 256 字节页步进擦 4KB 扇区，逻辑错误且占用 8KB 栈 | `SPI_FLASH.c` | 该函数无任何调用，已移除 |
| 中断里用 `printf`、调试函数依赖半主机 | `RTC.c` | 删除 |
| 秒计数 `s` 被中断写入却未加 `volatile` | `Tim.c` | 加 `volatile`；1 秒节拍改由 `SystemCoreClock` 推导（240 MHz 下寄存器值与原来完全一致） |

### 性能与资源

| 优化 | 效果 |
| --- | --- |
| 片内 Flash 写入改 32 位字编程 + 收尾字节编程 + 回读校验 | 128 KB 升级镜像的编程次数由约 13 万次降到约 3.3 万次 |
| `oled_clear_gram()`：只清显存不刷屏，状态页/上电画面各少一次整屏 IIC 写入 | 每次状态切换省约 20 ms |
| 页地址 + 列地址 3 个命令合并为一次 IIC 传输 | 一次整屏刷新省 8 次 START/STOP |
| Bootloader 上电整屏写入由 3 次降到 1 次 | 开机画面更快出现 |
| 十六进制解码直接在接收缓冲上完成 | 处理一帧少占 384 字节栈；Bootloader 的收帧缓冲移到静态区（栈 -384 B，`.bss` +384 B） |
| 删除未使用的旧版 AD3344 驱动、残留头文件等死代码 | 少一份与 `Analog.c` 冲突的 PA4/SPI0 定义 |

代码体积变化（同为 `-O2`、同一套 Makefile）：APP `text+data` 由 40676 B 变为 41008 B（+332 B），
Bootloader 由 8816 B 变为 9136 B（+320 B）——增量来自字编程回读校验、超时与边界检查等加固逻辑。

## 已知限制与后续可优化

* **Keil 工程仍是 `-O0`**（`<Optim>1</Optim>`），改成 `-O1/-O2` 可明显减小体积、提升速度；因为无法上机验证，本次没有改动 IDE 里的优化等级，只把它写在这里。
* **报警文本 `%.2f` 会引入约 11.6 KB 的浮点格式化代码**（实测：带 `-u _printf_float` 时 APP `.text` 40528 B，去掉后 28944 B）。若在意这 9% 的 Flash，可把报警文本改为整数定点格式化。
* **备份区 `0x08031000`（128 KB）尚未使用**，可作为“升级失败回滚”的目标区。
* 仓库没有单元测试与硬件在环测试，逻辑改动（尤其是 Flash 编程、低功耗、IAP 时序）需要上机复测。
* 固件里的标识符沿用原工程的拼音命名（`Xieyi_Chuli`、`Yanshi` 等），为保持与比赛模板一致未做重命名。

## 许可

应用逻辑按 [MIT](LICENSE) 发布；`Library/`、`CMSIS/` 下的 GigaDevice 库与 CMSIS 代码，
以及 `Driver/` 中源自正点原子例程的部分，遵循各自原始许可，详见 [THIRD_PARTY.md](THIRD_PARTY.md)。
