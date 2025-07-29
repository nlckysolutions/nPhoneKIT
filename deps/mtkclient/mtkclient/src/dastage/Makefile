ifneq (,$(findstring arm-linux-gnueabihf,$(shell gcc -dumpmachine)))
CC := gcc
AS := as
LD := gcc
OBJCOPY := objcopy
else
CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
endif

CFLAGS := -fPIE -std=gnu99 -Os -mthumb -mcpu=cortex-a9 -fno-builtin-printf -fno-strict-aliasing -fno-builtin-memcpy -mno-unaligned-access -Wall -Wextra
LDFLAGS := -fPIE -pie -nodefaultlibs -nostdlib -Wl,--build-id=none

PL := pl
PL_DIR := ../../build/pl
DSTPATH := ../../payloads
PL_BIN := $(DSTPATH)/$(PL).bin

PL_SRC = pl.c libc.c printf.c drivers/sd.c drivers/mmc.c crypto/sha256.c crypto/hmac-sha256.c
ASM_SRC = start.S

PL_OBJ = $(PL_SRC:%.c=$(PL_DIR)/%.o) $(ASM_SRC:%.S=$(PL_DIR)/%.o)
PL_DEP = $(PL_OBJ:%.o=%.d)

all: $(PL_DIR)/$(PL).bin
	mkdir -p $(DSTPATH)
	cp $(PL_DIR)/$(PL).bin $(PL_BIN)

$(PL_DIR)/$(PL).bin: $(PL_DIR)/$(PL).elf
	$(OBJCOPY) -O binary $^ $@

$(PL_DIR)/$(PL).elf: $(PL_OBJ)
	$(LD) -o $@ $^ $(LDFLAGS) -T $(PL).ld

-include $(PL_DEP)

$(PL_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -MMD -c -o $@ $< $(CFLAGS)

$(PL_DIR)/%.o: %.S
	mkdir -p $(@D)
	$(AS) -o $@ $<


clean:
	rm -rf $(PL_DIR)
	rm -f $(PL_BIN)