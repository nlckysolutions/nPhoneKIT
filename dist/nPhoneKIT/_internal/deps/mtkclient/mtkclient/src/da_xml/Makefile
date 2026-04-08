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

VPATH := %.h common crypto
INCLUDE_DIRS := .:$(PWD)/common:$(PWD)/crypto
DSTPATH := ../../payloads

CFLAGS := -std=gnu99 -Os -marm -mcpu=cortex-a9 -fno-builtin-printf -fno-strict-aliasing -fno-builtin-memcpy -fPIE -mno-unaligned-access -Wall -Wextra
LDFLAGS := -nodefaultlibs -nostdlib -Wl,--build-id=none


DA := da_xml
DA_BUILD := ../../build/da_xml

DA_BUILD_BIN := $(DSTPATH)/$(DA).bin

DA_SRC = da_xml.c common/libc.c crypto/sha256.c crypto/hmac-sha256.c
ASM_SRC = common/start.S

all: CFLAGS += -DNDEBUG
all: makepayloads

debug: CFLAGS += -DDEBUG -g
debug: makepayloads

makepayloads:  $(DA_BUILD)/da_xml.bin
	mkdir -p $(DSTPATH)
	cp $(DA_BUILD)/$(DA).bin $(DA_BUILD_BIN)

$(DA_BUILD)/$(DA).bin: $(DA_BUILD)/$(DA).elf
	$(OBJCOPY) -O binary $^ $@

$(DA_BUILD)/$(DA).elf: $(DA_SRC:%.c=$(DA_BUILD)/%.o) $(ASM_SRC:%.S=$(DA_BUILD)/%.o)
	$(LD) -o $@ $^ $(LDFLAGS) -T common/generic.ld

$(DA_BUILD)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

$(DA_BUILD)/%.o: %.S
	mkdir -p $(@D)
	$(AS) -o $@ $<

clean:
	-rm -rf $(DSTPATH)/da_xml.bin
