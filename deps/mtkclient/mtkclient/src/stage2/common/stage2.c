#include <inttypes.h>
//#define DEBUG 1
#include "tools.h"
#include "printf.h"
#include "libc.h"
#include "../drivers/sleepy.h"
#include "../drivers/types.h"
#include "../drivers/core.h"
#include "../drivers/mt_sd.h"
#include "../drivers/errno.h"
#include "../drivers/mmc.h"

extern void set_param(mcu* param);

void arch_enable_neon()
{
    uint32_t r3=0;
    asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r"(r3));
    r3 |= 0xF00000;
    asm volatile ("mcr p15, 0, %0, c1, c0, 2" :: "r"(r3));
    asm volatile ("VMRS %0, FPEXC" : "=r"(r3));
    r3 |= 0x40000000;
    asm volatile ("VMSR FPEXC, %0" :: "r"(r3));
}

__attribute__ ((section(".text.main"))) int main() {
    arch_enable_neon();
    volatile uint32_t address=0;
    uint32_t size=0;
    char* addressbuf=0;
    uint32_t block=0;
    uint32_t cmd=0;
    uint32_t magic=0;
    uint32_t part=0;
    char buf[0x200] = { 0 };
    int ret = 0;
    volatile uint32_t *reg=0;
    mcu param;
    uint16_t addr=0;
    uint32_t jump_addr=0;
    uint32_t rdsize=0;
    param.wdt = (volatile uint32_t *)0x10007000;
    param.uart_base = (volatile uint32_t *)0x11002000;
    param.uart_reg0 = 0;
    param.uart_reg1 = 0;
    param.send_usb_response = (void*)0x0;
    param.usbdl_get_data = (void*)0x0;
    param.usbdl_put_data = (void*)0x0;
    searchparams(&param);
    set_param(&param);

    printf("2ND stage payload\n");
    printf("(c) xyz, k4y0z, bkerler 2019-2021\n");

//    while(1) {}

    printf("Entering command loop\n");
    char buffer[0x200]={0};
    send_dword(&param,0xB1B2B3B4);
    struct msdc_host host = { 0 };
    host.ocr_avail = MSDC_OCR_AVAIL;
    hex_dump(&param, 0,0x16);
    int mmcinited=0;
    unsigned int pos=0;
    unsigned int count=0;
    while (1) {
        printf("Waiting for cmd\n");
        memset(buf, 0, sizeof(buf));
        magic = recv_dword(&param);
        if (magic != 0xf00dd00d) {
            printf("Protocol error\n");
            printf("Magic received = 0x%08X\n", magic);
            break;
        }
        cmd = recv_dword(&param);
        switch (cmd) {
        case 0x1000: {
            block = recv_dword(&param);
            count = recv_dword(&param);
            printf("Read block 0x%08X\n", block);
            memset(buf, 0, sizeof(buf));
            for (pos=block;pos<block+count;pos++)
            {
                if (mmc_read(&host, pos, buf) != 0) {
                    printf("Read error!\n");
                } else {
                    param.usbdl_put_data(buf, sizeof(buf));
                }
            }
            break;
        }
        case 0x1001: {
            block = recv_dword(&param);
            printf("Write block 0x%08X ", block);
            memset(buf, 0, sizeof(buf));
            param.usbdl_get_data(buf, 0x200);
            if (mmc_write(&host, block, buf) != 0) {
                printf("Write error!\n");
            } else {
                printf("OK\n");
                send_dword(&param,0xD0D0D0D0);
            }
            break;
        }
        case 0x1002: {
            part = recv_dword(&param);
            printf("Switch to partition %d => ", part);
            ret = mmc_set_part(&host, part);
            printf("0x%08X\n", ret);
            mdelay(500); // just in case
            break;
        }
        case 0x2000: {
            printf("Read rpmb\n");
            addr = (uint16_t)recv_word(&param);
            count = (uint16_t)recv_word(&param);
            for (pos=addr;pos<(unsigned int)addr+count;pos++)
            {
                if (mmc_rpmb_read(&host, (uint16_t)pos, buf)!=0)
                {
                    printf("Read error!\n");
                    break;
                }
                else
                {
                    param.usbdl_put_data(buf, 0x100);
                }
            }
            break;
        }
        case 0x2001: {
            printf("Write rpmb\n");
            param.usbdl_get_data(buf, 0x100);
            mmc_rpmb_write(&host, buf);
            break;
        }
        case 0x3000: {
            printf("Reboot\n");
            reg = (volatile uint32_t *)0x10007000;
            reg[8/4] = 0x1971;
            reg[0/4] = 0x22000014;
            reg[0x14/4] = 0x1209;
            while (1) {

            }
        }
        case 0x3001: {
            printf("Kick watchdog\n");
            reg = (volatile uint32_t *)0x10007000;
            reg[8/4] = 0x1971;
            break;
        }
        case 0x4001: {
            jump_addr=recv_dword(&param);
            #ifdef DEBUG
            print(&param,"Jump to ");
            pdword(&param,jump_addr);
            print(&param,"\n");
            #endif
            void (*jump)() = (void*)jump_addr;
            apmcu_icache_invalidate();
            apmcu_disable_icache();
            apmcu_isb();
            apmcu_disable_smp();
            print(&param,"JMP\n");
            jump();
            send_dword(&param,0xD0D0D0D0);
            break;
        }
        case 0x4002: {
            address = recv_dword(&param);
            size = recv_dword(&param);
            #ifdef DEBUG
                printf("Read %d Bytes from address 0x%08X\n", size, address);
            #endif
            param.usbdl_put_data(address, size);
            break;
        }
        case 0x4000: {
            addressbuf = (char*)recv_dword(&param);
            size = recv_dword(&param);
            rdsize=size;
            if (size%4!=0) rdsize=((size/4)+1)*4;
            recv_data(&param, buffer, rdsize, 0);
            if (size==4){
                // This is needed for registers to be written correctly
                *(volatile unsigned int *)(addressbuf) = *(unsigned int*)buffer;
                dsb();
                printf("Reg dword 0x%08X addr with value 0x%08X\n", address, *(unsigned int*)buffer);
            } else if (size==2){
                // This is needed for registers to be written correctly
                *(volatile unsigned short *)(addressbuf) = *(unsigned short*)buffer;
                dsb();
                printf("Reg short 0x%08X addr with value 0x%08X\n", address, *(unsigned short*)buffer);
            }
            else if (size==1){
                // This is needed for registers to be written correctly
                *(volatile unsigned char *)(addressbuf) = *(unsigned char*)buffer;
                dsb();
                printf("Reg byte 0x%08X addr with value 0x%08X\n", address, *(unsigned char*)buffer);
            }
            else {
                memcpy(addressbuf,buffer,size);
                }
            printf("Write %d Bytes to address 0x%08X\n", size, address);
            send_dword(&param, 0xD0D0D0D0);
            break;
        }
        case 0x5000: {
            apmcu_icache_invalidate();
            apmcu_disable_icache();
            apmcu_isb();
            apmcu_disable_smp();
            send_dword(&param, 0xD0D0D0D0);
            break;
        }
        case 0x6000: {
             mmc_init(&host);
             mmc_host_init(&host);
             send_dword(&param, 0xD1D1D1D1);
             mmcinited=1;
             break;
        }
        case 0x6001: {
             send_dword(&param, mmcinited);
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
