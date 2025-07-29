#include <inttypes.h>

#include "printf.h"

#include "libc.h"

#include "drivers/types.h"
#include "drivers/core.h"
#include "drivers/mt_sd.h"
#include "drivers/errno.h"
#include "drivers/mmc.h"

#define PRELOADER_BASE 0x201000

uint16_t send_dword_pattern[]  = {0xB507, 0x0E03};
uint16_t recv_dword_pattern[]  = {0x4B0E, 0x2200};
uint16_t msdc_init_pattern[]  = {0xB570, 0x1E05, 0x460B};

void (*send_dword)();
void (*_recv_dword)();

int recv_dword() {
    int dword = 0;
    _recv_dword(&dword);
    return dword;
}

void send_data(char *addr, uint32_t sz) {
    for (uint32_t i = 0; i < (((sz + 3) & ~3) / 4); i++) {
        send_dword(__builtin_bswap32(((uint32_t *)addr)[i]));
    }
}

void recv_data(char *addr, uint32_t sz, uint32_t flags __attribute__((unused))) {
    for (uint32_t i = 0; i < (((sz + 3) & ~3) / 4); i++) {
        ((uint32_t *)addr)[i] = __builtin_bswap32(recv_dword());
    }
}

uint16_t recv_word() {
    uint16_t word=recv_dword()&0xFFFF;
    return word;
}

void low_uart_put(int ch) {
    volatile uint32_t *uart_reg0 = (volatile uint32_t*)0x11002014;
    volatile uint32_t *uart_reg1 = (volatile uint32_t*)0x11002000;

    while ( !((*uart_reg0) & 0x20) )
    {}

    *uart_reg1 = ch;
}

void _putchar(char character)
{
    if (character == '\n')
        low_uart_put('\r');
    low_uart_put(character);
}

void sleepy(void) {
    // TODO: do better
    for (volatile int i = 0; i < 0x80000; ++i) {}
}

void mdelay (unsigned long msec)
{
    (void)msec;
    sleepy();
}

/* delay usec useconds */
void udelay (unsigned long usec)
{
    (void)usec;
    sleepy();
}

void hex_dump(const void* data, size_t size) {
    size_t i, j;
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("\n");
            } else if (i+1 == size) {
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("\n");
            }
        }
    }
}

uint32_t searchfunc(uint32_t startoffset, uint32_t endoffset, uint16_t *pattern, uint32_t patternsize) {
    uint32_t matched = 0;
    for (uint32_t offset = startoffset; offset < endoffset; offset += 2) {
        for (uint32_t i = 0; i < patternsize; i++) {
            if (((uint16_t *)offset)[i] != pattern[i]) {
                matched = 0;
                break;
            }
            if (++matched == patternsize) return offset;
        }
    }
    return 0;
}

__attribute__ ((section(".text.main"))) int main() {
    char buf[0x200] = { 0 };
    int ret = 0;
    int mmcinited = 0;
    printf("Generic preloader payload\n");
    printf("Copyright xyz, k4y0z, bkerler 2021\n");

    struct msdc_host host = { 0 };
    host.ocr_avail = MSDC_OCR_AVAIL;

    //mmc_init(&host);

    send_dword = (void *)(searchfunc(PRELOADER_BASE + 0x100, PRELOADER_BASE + 0x20000, send_dword_pattern, 2) | 1);
    printf("send_dword = %p\n", send_dword);
    _recv_dword = (void *)(searchfunc(PRELOADER_BASE + 0x100, PRELOADER_BASE + 0x20000, recv_dword_pattern, 2) | 1);
    printf("_recv_dword = %p\n", _recv_dword);

    //void (*msdc_init)(uint32_t card, uint32_t unk) = (void *)(searchfunc(PRELOADER_BASE + 0x100, PRELOADER_BASE + 0x20000, msdc_init_pattern, 3) | 1);
    //printf("msdc_init = %p\n", msdc_init);
    //msdc_init(0, 1);

    //void (*msdc_init)(uint32_t a, uint32_t b) = (void *)0x21E569;
    //msdc_init(0,1);
    //unsigned int a1=0;
    //unsigned int a2=1;

    //mmc_host_init(&host);

    //void (*mmc_init_host)(uint32_t a, uint32_t b, uint32_t c, uint32_t d) = (void *)0x21E545;
    //mmc_init_host((a1 << 7) + 0x10A450, a1, -1, a2);

    //void (*mmc_init_card)(uint32_t a, uint32_t b) = (void *)0x21E481;
    //mmc_init_card((a1 << 7) + 0x10A450, 740 * a1 + 0x10A16C);

    printf("Entering command loop\n");

    send_dword(0xB1B2B3B4);

    while (1) {
        memset(buf, 0, sizeof(buf));
        uint32_t magic = recv_dword();
        if (magic != 0xf00dd00d) {
            printf("Protocol error\n");
            printf("Magic received = 0x%08X\n", magic);
            break;
        }
        uint32_t cmd = recv_dword();
        switch (cmd) {
        case 0x1000: {
            uint32_t block = recv_dword();
            printf("Read block 0x%08X\n", block);
            memset(buf, 0, sizeof(buf));
            if (mmc_read(&host, block, buf) != 0) {
                printf("Read error!\n");
            } else {
                send_data(buf, sizeof(buf));
            }
            break;
        }
        case 0x1001: {
            uint32_t block = recv_dword();
            printf("Write block 0x%08X ", block);
            memset(buf, 0, sizeof(buf));
            recv_data(buf, 0x200, 0);
            if (mmc_write(&host, block, buf) != 0) {
                printf("Write error!\n");
            } else {
                printf("OK\n");
                send_dword(0xD0D0D0D0);
            }
            break;
        }
        case 0x1002: {
            uint32_t part = recv_dword();
            printf("Switch to partition %d => ", part);
            ret = mmc_set_part(&host, part);
            printf("0x%08X\n", ret);
            mdelay(500); // just in case
            break;
        }
        case 0x2000: {
            printf("Read rpmb\n");
            uint16_t addr = (uint16_t)recv_word();
            mmc_rpmb_read(&host, addr, buf);
            send_data(buf, 0x100);
            break;
        }
        case 0x2001: {
            printf("Write rpmb\n");
            uint16_t addr = (uint16_t)recv_word();
            recv_data(buf, 0x100, 0);
            mmc_rpmb_write(&host, addr, buf);
            break;
        }
        case 0x3000: {
            printf("Reboot\n");
            volatile uint32_t *reg = (volatile uint32_t *)0x10007000;
            reg[8/4] = 0x1971;
            reg[0/4] = 0x22000014;
            reg[0x14/4] = 0x1209;

            while (1) {

            }
        }
        case 0x3001: {
            printf("Kick watchdog\n");
            volatile uint32_t *reg = (volatile uint32_t *)0x10007000;
            reg[8/4] = 0x1971;
            break;
        }
        case 0x4001: {
            uint32_t jump_addr=recv_dword();
            #ifdef DEBUG
            printf("Jump to ");
            pdword(jump_addr);
            printf("\n");
            #endif
            void (*jump)() = (void*)jump_addr;
            /*apmcu_icache_invalidate();
            apmcu_disable_icache();
            apmcu_isb();
            apmcu_disable_smp();*/
            printf("JMP\n");
            jump();
            send_dword(0xD0D0D0D0);
            break;
        }
        case 0x4002: {
            uint32_t address = recv_dword();
            uint32_t size = recv_dword();
            //printf("Read %d Bytes from address 0x%08X\n", size, address);
            send_data((char*)address, size);
            break;
        }
        case 0x4000: {
            char* address = (char*)recv_dword();
            uint32_t size = recv_dword();
            uint32_t rdsize=size;
            if (size%4!=0) rdsize=((size/4)+1)*4;
            recv_data(buf, rdsize, 0);
            if (size==4){
                // This is needed for registers to be written correctly
                *(volatile unsigned int *)(address) = *(unsigned int*)buf;
                dsb();
                printf("Reg dword 0x%08X addr with value 0x%08X\n", address, *(unsigned int*)buf);
            } else if (size==2){
                // This is needed for registers to be written correctly
                *(volatile unsigned short *)(address) = *(unsigned short*)buf;
                dsb();
                printf("Reg short 0x%08X addr with value 0x%08X\n", address, *(unsigned short*)buf);
            }
            else if (size==1){
                // This is needed for registers to be written correctly
                *(volatile unsigned char *)(address) = *(unsigned char*)buf;
                dsb();
                printf("Reg byte 0x%08X addr with value 0x%08X\n", address, *(unsigned char*)buf);
            }
            else {
                memcpy(address,buf,size);
                }
            printf("Write %d Bytes to address 0x%08X\n", size, address);
            send_dword(0xD0D0D0D0);
            break;
        }
        case 0x6000: {
             mmc_init(&host);
             mmc_host_init(&host);
             send_dword(0xD1D1D1D1);
             mmcinited=1;
             break;
        }
        case 0x6001: {
             send_dword(mmcinited);
             break;
        }
        default:
            printf("Invalid command\n");
            break;
        }
    }

    printf("Exiting the payload\n");

    while (1) {

    }
}
