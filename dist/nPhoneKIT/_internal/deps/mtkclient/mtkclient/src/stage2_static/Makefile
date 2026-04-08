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

CFLAGS := -std=gnu99 -Os -fpic -nostdlib -mthumb -mcpu=cortex-a9 -fno-builtin-printf -fno-strict-aliasing -fno-builtin-memcpy -mno-unaligned-access -Wall -Wextra
LDFLAGS := -nodefaultlibs -nostdlib -Wl,--build-id=none

STAGE2 := stage2
STAGE2DST := ../../build/stage2
DSTPATH := ../../payloads
STAGE2DST_BIN := $(DSTPATH)/$(STAGE2).bin

STAGE2_SRC = stage2.c tools.c libc.c printf.c drivers/sleepy.c drivers/sd.c drivers/mmc.c crypto/sha256.c crypto/hmac-sha256.c
ASM_SRC = start.S

STAGE2_OBJ = $(STAGE2_SRC:%.c=$(STAGE2DST)/%.o) $(ASM_SRC:%.S=$(STAGE2DST)/%.o)
STAGE2_DEP = $(STAGE2_OBJ:%.o=%.d)

all:  $(STAGE2DST)/$(STAGE2).bin
	mkdir -p $(DSTPATH)
	cp $(STAGE2DST)/$(STAGE2).bin $(STAGE2DST_BIN)

$(STAGE2DST)/$(STAGE2).bin: $(STAGE2DST)/$(STAGE2).elf
	$(OBJCOPY) -O binary $^ $@

$(STAGE2DST)/$(STAGE2).elf: $(STAGE2_OBJ)
	$(LD) -o $@ $^ $(LDFLAGS) -T $(STAGE2).ld

-include $(STAGE2_DEP)

$(STAGE2DST)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -MMD -c -o $@ $< $(CFLAGS)

$(STAGE2DST)/%.o: %.S
	mkdir -p $(@D)
	$(AS) -o $@ $<

clean:
	-rm -rf $(STAGE2DST)
	-rm -f $(STAGE2DST_BIN)
