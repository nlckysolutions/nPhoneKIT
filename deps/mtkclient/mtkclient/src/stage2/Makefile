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

VPATH := %.h common crypto drivers
INCLUDE_DIRS := .:$(PWD)/common:$(PWD)/crypto:$(PWD)/drivers
DSTPATH := ../../payloads

CFLAGS := -fPIC -z text -std=gnu99 -Os -mthumb -mcpu=cortex-a9 -fno-builtin-printf -fno-strict-aliasing -fno-builtin-memcpy -Wall -Wextra
LDFLAGS := -nodefaultlibs -nostdlib -Wl,--build-id=none

STAGE2 := stage2
STAGE2DST := ../../build/stage2

STAGE2DST_BIN := $(DSTPATH)/$(STAGE2).bin

STAGE2_SRC = common/stage2.c common/tools.c common/libc.c common/printf.c drivers/sleepy.c drivers/sd.c drivers/mmc.c crypto/sha256.c crypto/hmac-sha256.c
ASM_SRC = common/start.S

all:  $(STAGE2DST)/stage2.bin
	mkdir -p $(DSTPATH)
	cp $(STAGE2DST)/$(STAGE2).bin $(STAGE2DST_BIN)

$(STAGE2DST)/$(STAGE2).bin: $(STAGE2DST)/$(STAGE2).elf
	$(OBJCOPY) -O binary $^ $@

$(STAGE2DST)/stage2.elf: $(STAGE2_SRC:%.c=$(STAGE2DST)/%.o) $(ASM_SRC:%.S=$(STAGE2DST)/%.o)
	$(LD) -o $@ $^ $(LDFLAGS) -T common/stage2.ld

$(STAGE2DST)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

$(STAGE2DST)/%.o: %.S
	mkdir -p $(@D)
	$(AS) -o $@ $<

clean:
	-rm -rf $(STAGE2DST)
	-rm -f $(STAGE2DST_BIN)
