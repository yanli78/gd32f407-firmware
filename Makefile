# ============================================================================
#  GD32F470VE firmware — APP + Bootloader
#  GNU Arm Embedded Toolchain build (alternative to the Keil MDK projects)
#
#  Usage:
#      make            # build APP and Bootloader (elf + hex + bin)
#      make app        # build only the application
#      make boot       # build only the bootloader
#      make size       # print section sizes
#      make clean      # remove build/
#
#  Options:
#      CROSS=arm-none-eabi-          toolchain prefix
#      OPT=-O2                       optimisation flags
# ============================================================================

CROSS    ?= arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
SIZE     := $(CROSS)size

# ------------------------------------------------- portable shell helpers ---
ifeq ($(OS),Windows_NT)
  MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
  RMDIR = if exist "$(subst /,\,$1)" rmdir /s /q "$(subst /,\,$1)"
else
  MKDIR = mkdir -p "$1"
  RMDIR = rm -rf "$1"
endif

# ----------------------------------------------------------------- toolchain
CPUFLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
OPT      ?= -O2
WARN     := -Wall -Wextra
CFLAGS   := $(CPUFLAGS) $(OPT) -std=gnu11 $(WARN) -g3 \
            -ffunction-sections -fdata-sections -fno-strict-aliasing
LDFLAGS  := $(CPUFLAGS) $(OPT) -nostartfiles -Wl,--gc-sections
LDLIBS   := --specs=nano.specs --specs=nosys.specs -lc

DEFS     := -DUSE_STDPERIPH_DRIVER -DGD32F470

APP      := APP
BOOT     := Bootloader
LIB      := Library/GD32F4xx_standard_peripheral/Source
LIB_INC  := Library/GD32F4xx_standard_peripheral/Include
CMSIS    := CMSIS/GD/GD32F4xx

# ------------------------------------------------------------------ app config
APP_NAME   := CIMC_GD32_Template
APP_DEFS   := $(DEFS) -DAPP_IMAGE
# nano.specs omits float support in printf/snprintf by default; the alarm
# records are formatted with "%.2f", so pull the float formatter back in.
APP_LDLIBS := $(LDLIBS) -u _printf_float
APP_LD     := $(APP)/project/gd32f470ve_app.ld
APP_INC    := \
	$(APP)/CMSIS \
	$(APP)/$(CMSIS)/Include \
	$(APP)/$(LIB_INC) \
	$(APP)/User \
	$(APP)/HeaderFiles \
	$(APP)/Function \
	$(APP)/Protocol \
	$(APP)/Startup \
	$(APP)/Driver/ADC \
	$(APP)/Driver/Flash \
	$(APP)/Driver/IIC \
	$(APP)/Driver/LED \
	$(APP)/Driver/OLED \
	$(APP)/Driver/RTC \
	$(APP)/Driver/System \
	$(APP)/Driver/Timer \
	$(APP)/Driver/UART

APP_SRC := \
	$(APP)/User/main.c \
	$(APP)/User/gd32f4xx_it.c \
	$(APP)/User/systick.c \
	$(APP)/User/syscalls.c \
	$(APP)/Function/Function.c \
	$(APP)/Function/fun.c \
	$(APP)/Function/AppService.c \
	$(APP)/Protocol/Protocol.c \
	$(APP)/Driver/ADC/Analog.c \
	$(APP)/Driver/ADC/pt100_spi.c \
	$(APP)/Driver/Flash/SPI_FLASH.c \
	$(APP)/Driver/IIC/myiic.c \
	$(APP)/Driver/LED/LED.c \
	$(APP)/Driver/OLED/OLED.c \
	$(APP)/Driver/RTC/RTC.c \
	$(APP)/Driver/System/sys.c \
	$(APP)/Driver/Timer/Tim.c \
	$(APP)/Driver/UART/RS485.c \
	$(APP)/$(CMSIS)/Source/system_gd32f4xx.c \
	$(APP)/$(LIB)/gd32f4xx_adc.c \
	$(APP)/$(LIB)/gd32f4xx_dac.c \
	$(APP)/$(LIB)/gd32f4xx_exti.c \
	$(APP)/$(LIB)/gd32f4xx_fmc.c \
	$(APP)/$(LIB)/gd32f4xx_gpio.c \
	$(APP)/$(LIB)/gd32f4xx_misc.c \
	$(APP)/$(LIB)/gd32f4xx_pmu.c \
	$(APP)/$(LIB)/gd32f4xx_rcu.c \
	$(APP)/$(LIB)/gd32f4xx_rtc.c \
	$(APP)/$(LIB)/gd32f4xx_spi.c \
	$(APP)/$(LIB)/gd32f4xx_syscfg.c \
	$(APP)/$(LIB)/gd32f4xx_timer.c \
	$(APP)/$(LIB)/gd32f4xx_usart.c
APP_ASM := $(APP)/Startup/startup_gd32f470_gcc.s

# ----------------------------------------------------------------- boot config
BOOT_NAME := CIMC_BOOT
BOOT_DEFS := $(DEFS)
BOOT_LD   := $(BOOT)/project/gd32f470ve_boot.ld
BOOT_INC  := \
	$(BOOT)/CMSIS \
	$(BOOT)/$(CMSIS)/Include \
	$(BOOT)/$(LIB_INC) \
	$(BOOT)/User \
	$(BOOT)/HeaderFiles \
	$(BOOT)/Function \
	$(BOOT)/Protocol \
	$(BOOT)/Startup \
	$(BOOT)/Driver

BOOT_SRC := \
	$(BOOT)/Function/boot_main.c \
	$(BOOT)/User/boot_it.c \
	$(BOOT)/Protocol/Boot_Protocol.c \
	$(BOOT)/Driver/Boot_Flash.c \
	$(BOOT)/Driver/Boot_OLED.c \
	$(BOOT)/Driver/Boot_RS485.c \
	$(BOOT)/Driver/Boot_Timer.c \
	$(BOOT)/$(CMSIS)/Source/system_gd32f4xx.c \
	$(BOOT)/$(LIB)/gd32f4xx_fmc.c \
	$(BOOT)/$(LIB)/gd32f4xx_gpio.c \
	$(BOOT)/$(LIB)/gd32f4xx_misc.c \
	$(BOOT)/$(LIB)/gd32f4xx_rcu.c \
	$(BOOT)/$(LIB)/gd32f4xx_usart.c
BOOT_ASM := $(BOOT)/Startup/startup_gd32f470_gcc.s

# ------------------------------------------------------------- build layout --
BDIR      := build
APP_ODIR  := $(BDIR)/app
BOOT_ODIR := $(BDIR)/boot

APP_OBJ   := $(addprefix $(APP_ODIR)/,$(notdir $(APP_SRC:.c=.o))) \
             $(addprefix $(APP_ODIR)/,$(notdir $(APP_ASM:.s=.o)))
BOOT_OBJ  := $(addprefix $(BOOT_ODIR)/,$(notdir $(BOOT_SRC:.c=.o))) \
             $(addprefix $(BOOT_ODIR)/,$(notdir $(BOOT_ASM:.s=.o)))

APP_ELF   := $(BDIR)/$(APP_NAME).elf
BOOT_ELF  := $(BDIR)/$(BOOT_NAME).elf

# ------------------------------------------------------ per-source C rules ---
# Explicit rules (instead of a pattern rule) so objects stay in a flat
# directory and no per-file mkdir is needed on Windows.
define APP_C_RULE
$(APP_ODIR)/$(notdir $(1:.c=.o)): $(1) | $(APP_ODIR)
	$(CC) -c $(CFLAGS) $(APP_DEFS) $(addprefix -I,$(APP_INC)) $$< -o $$@
endef
define BOOT_C_RULE
$(BOOT_ODIR)/$(notdir $(1:.c=.o)): $(1) | $(BOOT_ODIR)
	$(CC) -c $(CFLAGS) $(BOOT_DEFS) $(addprefix -I,$(BOOT_INC)) $$< -o $$@
endef
$(foreach s,$(APP_SRC),$(eval $(call APP_C_RULE,$(s))))
$(foreach s,$(BOOT_SRC),$(eval $(call BOOT_C_RULE,$(s))))

# --------------------------------------------------------------------- rules
.PHONY: all app boot hex bin size clean help

all: app boot

app: $(APP_ELF) $(APP_ELF:.elf=.hex) $(APP_ELF:.elf=.bin)
boot: $(BOOT_ELF) $(BOOT_ELF:.elf=.hex) $(BOOT_ELF:.elf=.bin)

hex: $(APP_ELF:.elf=.hex) $(BOOT_ELF:.elf=.hex)
bin: $(APP_ELF:.elf=.bin) $(BOOT_ELF:.elf=.bin)

$(APP_ODIR) $(BOOT_ODIR) $(BDIR):
	@$(call MKDIR,$@)

$(APP_ELF): $(APP_OBJ) $(APP_LD) | $(BDIR)
	$(CC) $(APP_OBJ) $(LDFLAGS) -T$(APP_LD) -Wl,-Map=$(@:.elf=.map) \
		-Wl,--print-memory-usage $(APP_LDLIBS) -o $@

$(BOOT_ELF): $(BOOT_OBJ) $(BOOT_LD) | $(BDIR)
	$(CC) $(BOOT_OBJ) $(LDFLAGS) -T$(BOOT_LD) -Wl,-Map=$(@:.elf=.map) \
		-Wl,--print-memory-usage $(LDLIBS) -o $@

$(APP_ODIR)/$(notdir $(APP_ASM:.s=.o)): $(APP_ASM) | $(APP_ODIR)
	$(CC) -c $(CPUFLAGS) -x assembler-with-cpp $< -o $@

$(BOOT_ODIR)/$(notdir $(BOOT_ASM:.s=.o)): $(BOOT_ASM) | $(BOOT_ODIR)
	$(CC) -c $(CPUFLAGS) -x assembler-with-cpp $< -o $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

size: all
	@echo "=== $(APP_NAME) ==="
	@$(SIZE) $(APP_ELF)
	@echo "=== $(BOOT_NAME) ==="
	@$(SIZE) $(BOOT_ELF)

clean:
	@$(call RMDIR,$(BDIR))

help:
	@echo "make [all|app|boot|hex|bin|size|clean]"
	@echo "  CROSS=<prefix>   toolchain prefix (default arm-none-eabi-)"
	@echo "  OPT=<flags>      optimisation flags (default -O2)"
