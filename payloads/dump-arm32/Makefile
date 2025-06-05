
CROSS ?= $(CROSS_AARCH32_DIR)arm-none-eabi-

AS  := $(CROSS)gcc -x assembler-with-cpp
CC  := $(CROSS)gcc
CXX := $(CROSS)g++
LD  := $(CROSS)ld
OBJCOPY := $(CROSS)objcopy


AS_FLAGS  := -Wall -O3 -ffunction-sections -fdata-sections -ffreestanding -std=gnu99 -fno-ident
CF_LAGS   := -Wall -O3 -ffunction-sections -fdata-sections -ffreestanding -std=gnu99 -fno-ident
CXX_FLAGS := -Wall -O3 -ffunction-sections -fdata-sections -ffreestanding -fno-ident
LD_FLAGS  := -Wl,-gc-sections -T link.ld -nostartfiles -nostdinc -nostdlib
MC_FLAGS  := -march=armv7-a -mtune=cortex-a7 -mfpu=vfpv4 -mfloat-abi=softfp -marm -mno-thumb-interwork -mno-unaligned-access -fno-stack-protector


all:
	@$(CC) $(CF_LAGS)  $(MC_FLAGS) -c ./src/hexdump.c -o ./hexdump.o
	@$(AS) $(AS_FLAGS) $(MC_FLAGS) -c ./src/start.S -o ./start.o
	@$(CC) $(LD_FLAGS) ./start.o ./hexdump.o -o ./dump-arm32.elf
	@$(OBJCOPY) -v -O binary ./dump-arm32.elf ./dump-arm32.bin

