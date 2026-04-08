#include <inttypes.h>

//#include "common.h"
#include "tools.h"
#include "printf.h"
#include "libc.h"
#include "drivers/sleepy.h"
#include "drivers/types.h"
#include "drivers/core.h"
#include "drivers/mt_sd.h"
#include "drivers/errno.h"
#include "drivers/mmc.h"

void recv_data(char *addr, uint32_t sz, uint32_t flags __attribute__((unused))) {
    for (uint32_t i = 0; i < (((sz + 3) & ~3) / 4); i++) {
        ((uint32_t *)addr)[i] = __builtin_bswap32(recv_dword());
    }
}

/*
void cache1()
{
  unsigned int v0; // r0
  unsigned int v1; // r0
  unsigned int v2; // r10
  unsigned int v3; // r1
  char v4; // r5
  int v5; // r7
  int v6; // r9
  bool v7; // cc

  asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(v0));
  v0 = v0 & 0xFFFFFFFB;
  asm volatile ("dsb 0xF":::"memory");
  asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r"(v0));
  asm volatile ("dsb 0xF":::"memory");
  asm volatile ("isb 0xF":::"memory");
  asm volatile ("mcr p15, 0, %0, c8, c7, 1" :: "r"(v0));
  asm volatile ("dsb 0xF":::"memory");
  asm volatile ("isb 0xF":::"memory");
  asm volatile ("dmb 0xF":::"memory");
  asm volatile ("mrc p15, 1, %0, c0, c0, 1" : "=r"(v1));
  if ( (v1 & 0x7000000) != 0 )
  {
    v2 = 0;
    do
    {
      if ( ((v1 >> (v2 + (v2 >> 1))) & 7) >= 2 )
      {
        asm volatile ("mcr p15, 2, %0, c0, c0, 0" :: "r"(v2));
        asm volatile ("dsb 0xF":::"memory");
        asm volatile ("isb 0xF":::"memory");
        asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r"(v3));
        asm volatile("clz    %0, %1 \n" : "=r" (v4) : "r"((v3 >> 3) & 0x3FF));
        v5 = (v3 >> 13) & 0x7FFF;
        do
        {
          v6 = (v3 >> 3) & 0x3FF;
          do
          {
            asm volatile ("mcr p15, 0, %0, c7, c14, 2" :: "r"(v2 | (v6 << v4) | (v5 << ((v3 & 7) + 4))));
            v7 = v6-- < 1;
          }
          while ( !v7 );
          v7 = v5-- < 1;
        }
        while ( !v7 );
      }
      v2 += 2;
    }
    while ( (int)((v1 & 0x7000000) >> 23) > (int)v2 );
  }
  asm volatile ("mcr p15, 2, %0, c0, c0, 0" :: "r"(0));
  asm volatile ("dsb 0xF":::"memory");
  asm volatile ("isb 0xF":::"memory");
}

void cache2(int flag){
            uint32_t tmp;
            uint32_t val;
            asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
            if (flag) val=tmp|0x1000;
            else val=tmp&0xFFFFEFFF;
            asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r"(val));
            asm volatile ("dsb 0xF":::"memory");
            asm volatile ("isb 0xF":::"memory");
}

void cache3(){
    asm volatile ("mcr p15, 0, %0, c7, c1, 0" :: "r"(0));
    asm volatile ("dsb 0xF":::"memory");
    asm volatile ("isb 0xF":::"memory");
}
*/

void apmcu_icache_invalidate(){
    asm volatile ("mcr p15, 0, %0, c7, c5, 0" :: "r"(0));
}

void apmcu_isb(){
    asm volatile ("ISB");
}

void apmcu_disable_icache(){
    uint32_t r0=0;
    asm volatile ("mcr p15, 0, %0, c7, c5, 6" :: "r"(r0)); /* Flush entire branch target cache */
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(r0));
    asm volatile ("bic %0,%0,#0x1800" : "=r"(r0) : "r"(r0)); /* I+Z bits */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r"(r0));
}

void apmcu_disable_smp(){
    uint32_t r0=0;
    asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r"(r0));
    asm volatile ("bic %0,%0,#0x40" : "=r"(r0) : "r"(r0)); /* SMP bit */
    asm volatile ("mcr p15, 0, %0, c1, c0, 1" :: "r"(r0));
}

int main() {
    searchparams();
    char buf[0x200] = { 0 };
    int ret = 0;

    printf("2ND stage payload\n");
    printf("(c) xyz, k4y0z, bkerler 2019-2021\n");

//    while(1) {}

    printf("Entering command loop\n");
    char buffer[0x200]={0};
    send_dword(0xB1B2B3B4);
    struct msdc_host host = { 0 };
    host.ocr_avail = MSDC_OCR_AVAIL;

    while (1) {
        printf("Waiting for cmd\n");
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
                usbdl_put_data(buf, sizeof(buf));
            }
            break;
        }
        case 0x1001: {
            uint32_t block = recv_dword();
            printf("Write block 0x%08X ", block);
            memset(buf, 0, sizeof(buf));
            usbdl_get_data(buf, 0x200, 0);
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
            usbdl_put_data(buf, 0x100);
            break;
        }
        case 0x2001: {
            printf("Write rpmb\n");
            usbdl_get_data(buf, 0x100, 0);
            mmc_rpmb_write(&host, buf);
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
        case 0x4002: {
            uint32_t address = recv_dword();
            uint32_t size = recv_dword();
            printf("Read %d Bytes from address 0x%08X\n", size, address);
            usbdl_put_data(address, size);
            break;
        }
        case 0x4000: {
            char* address = (char*)recv_dword();
            uint32_t size = recv_dword();
            uint32_t rdsize=size;
            if (size%4!=0) rdsize=((size/4)+1)*4;
            recv_data(buffer, rdsize, 0);
            if (size==4){
                // This is needed for registers to be written correctly
                *(volatile unsigned int *)(address) = *(unsigned int*)buffer;
                dsb();
                printf("Reg dword 0x%08X addr with value 0x%08X\n", address, *(unsigned int*)buffer);
            } else if (size==2){
                // This is needed for registers to be written correctly
                *(volatile unsigned short *)(address) = *(unsigned short*)buffer;
                dsb();
                printf("Reg short 0x%08X addr with value 0x%08X\n", address, *(unsigned short*)buffer);
            }
            else if (size==1){
                // This is needed for registers to be written correctly
                *(volatile unsigned char *)(address) = *(unsigned char*)buffer;
                dsb();
                printf("Reg byte 0x%08X addr with value 0x%08X\n", address, *(unsigned char*)buffer);
            }
            else {
                memcpy(address,buffer,size);
                }
            printf("Write %d Bytes to address 0x%08X\n", size, address);
            send_dword(0xD0D0D0D0);
            break;
        }
        case 0x5000: {
            apmcu_icache_invalidate();
            apmcu_disable_icache();
            apmcu_isb();
            apmcu_disable_smp();
            send_dword(0xD0D0D0D0);
            break;
        }
        case 0x6000: {
             mmc_init(&host);
             mmc_host_init(&host);
             send_dword(0xD1D1D1D1);
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
