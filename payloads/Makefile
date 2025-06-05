.PHONY: all clean

cross_32_dir := $(CROSS_DIR)
cross_64_dir := $(CROSS_AARCH64_DIR)

AS      = $(cross_32_dir)arm-none-eabi-as.exe
OBJCOPY = $(cross_dir)arm-none-eabi-objcopy.exe

AS64      = $(cross_64_dir)aarch64-none-elf-as.exe
OBJCOPY64 = $(cross_64_dir)aarch64-none-elf-objcopy.exe

all: write-arm32 exec-arm32 write-arm64 exec-arm64


# windows/linux
ifeq ($(shell echo "check_quotes"),"check_quotes")
   mkdir = mkdir $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
   rm    = $(wordlist 2,65535,$(foreach FILE,$(subst /,\,$(1)),& del /S /Q /F $(FILE) > nul 2>&1)) || (exit 0)
   rmdir = rmdir /s /q $(subst /,\,$(1)) > nul 2>&1 || (exit 0)
   echo  = echo $(1)
else
   mkdir = mkdir -p $(1)
   rm    = rm $(1) > /dev/null 2>&1 || true
   rmdir = rmdir $(1) > /dev/null 2>&1 || true
   echo  = echo "$(1)"
endif

write-arm32:
	@$(AS) -c write-arm32.S -o write-arm32.o
	@$(OBJCOPY) -O binary write-arm32.o write-arm32.bin

write-arm64:
	@$(AS64) -mcpu=cortex-a53 -c write-arm64.S -o write-arm64.o
	@$(OBJCOPY64) -O binary write-arm64.o write-arm64.bin

exec-arm32:
	@$(AS) -c exec-arm32.S -o exec-arm32.o
	@$(OBJCOPY) -O binary exec-arm32.o exec-arm32.bin

exec-arm64:
	@$(AS64) -mcpu=cortex-a53 -c exec-arm64.S -o exec-arm64.o
	@$(OBJCOPY64) -O binary exec-arm64.o exec-arm64.bin

clean:
	@$(call rm, -fr *.o)
	@$(call rm, -fr *.bin)
	@$(call rm, -fr *.elf)

rm_o:
	@$(call rm, -fr *.o)
	@$(call rm, -fr *.elf)
