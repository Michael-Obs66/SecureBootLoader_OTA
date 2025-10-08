# ===========================================================
# Makefile.mak - Build script for STM32L4 Secure Bootloader
# Author: Michael-Obs66
# Description: Generic Makefile for STM32L4 Bootloader/Application
# ===========================================================

###############################
# Toolchain
###############################
CC       = arm-none-eabi-gcc
AS       = arm-none-eabi-as
CP       = arm-none-eabi-objcopy
SZ       = arm-none-eabi-size
LD       = arm-none-eabi-gcc
HEX      = arm-none-eabi-objcopy
RM       = rm -f

###############################
# Project configuration
###############################
TARGET    = bootloader            # change to 'application' for app build
BUILD_DIR = build
LINKER_SCRIPT = ../platform/stm32l4/stm32l4.ld

###############################
# MCU Specific flags
###############################
CPU     = -mcpu=cortex-m4
FPU     = -mfpu=fpv4-sp-d16
FLOAT   = -mfloat-abi=hard
MCU     = $(CPU) -mthumb $(FPU) $(FLOAT)

###############################
# Directories
###############################
SRC_DIRS = . \
           ../crypto \
           ../utils \
           ../platform/stm32l4

INCLUDE_DIRS = -I. \
               -I../crypto \
               -I../utils \
               -I../platform/stm32l4 \
               -I../platform/stm32l4/CMSIS \
               -I../platform/stm32l4/HAL

###############################
# Compiler/Linker Flags
###############################
CFLAGS  = $(MCU) -Wall -Werror -Os -g3 \
          -ffunction-sections -fdata-sections \
          $(INCLUDE_DIRS) \
          -DSTM32L4xx

LDFLAGS = $(MCU) -Wl,--gc-sections \
          -T$(LINKER_SCRIPT)

###############################
# Source files
###############################
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

###############################
# Output files
###############################
ELF = $(BUILD_DIR)/$(TARGET).elf
BIN = $(BUILD_DIR)/$(TARGET).bin
HEX_FILE = $(BUILD_DIR)/$(TARGET).hex

###############################
# Build Rules
###############################
all: $(BUILD_DIR) $(BIN) size

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(foreach dir,$(SRC_DIRS),$(BUILD_DIR)/$(dir))

# Compile each C file
$(BUILD_DIR)/%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Link object files
$(ELF): $(OBJS)
	@echo "[LD] Linking..."
	@$(LD) $(OBJS) $(LDFLAGS) -o $(ELF)

# Create binary and hex
$(BIN): $(ELF)
	@echo "[CP] Converting ELF -> BIN"
	@$(CP) -O binary $(ELF) $(BIN)
	@$(HEX) -O ihex $(ELF) $(HEX_FILE)

# Show size of output
size:
	@$(SZ) $(ELF)

# Clean build
clean:
	@echo "[CLEAN] Removing build directory..."
	@$(RM) -r $(BUILD_DIR)

# Flash using st-flash
flash: all
	@echo "[FLASH] Writing to STM32L4 flash..."
	st-flash write $(BIN) 0x08000000

.PHONY: all clean flash size
