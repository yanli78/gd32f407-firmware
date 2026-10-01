# 第三方代码说明

本仓库的应用逻辑（`APP/User`、`APP/Function`、`APP/Protocol`、`APP/Driver` 中自有部分、
`Bootloader/Function`、`Bootloader/Protocol`、`Bootloader/Driver`）按根目录 `LICENSE`（MIT）发布。

以下代码来自第三方，保留其原始许可与版权，**不在** MIT 授权范围内：

| 路径 | 来源 | 许可 |
| --- | --- | --- |
| `APP/Library/`、`Bootloader/Library/` | GigaDevice GD32F4xx 标准外设库（V2.6.x） | BSD-3-Clause, Copyright (c) GigaDevice Semiconductor Inc. |
| `APP/CMSIS/`、`Bootloader/CMSIS/` | ARM CMSIS Core 与 GD32F4xx 器件支持文件、厂商启动文件 | Apache-2.0 (CMSIS) / BSD-3-Clause (GigaDevice) |
| `APP/Driver/OLED/oledfont.h` | OLED ASCII/汉字点阵字库 | 源自正点原子（ALIENTEK）例程 |
| `APP/Driver/{OLED,IIC,RTC}/`、`APP/Driver/System/` | 显示、IIC、RTC、系统延时驱动框架 | 源自正点原子（ALIENTEK）GD32F470 例程 |
| `APP/Driver/Flash/` | SPI Flash（GD25Q 系列）驱动 | 源自 GigaDevice 官方例程 |
| `APP/User/syscalls.c`、`APP/User/systick.c` | newlib 桩函数、SysTick 延时 | GigaDevice 例程 / newlib 惯例实现 |

如果要把本工程用于商业发布，请自行确认上述第三方组件的授权条件。
