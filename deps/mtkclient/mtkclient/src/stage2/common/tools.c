#include "printf.h"
#include "tools.h"
// (c) 2021 by bkerler
// #define DEBUG 1
static const uint32_t brom_bases[3] = {0, 0x00400000, 0x48000000};
static const char hex[] = "0123456789ABCDEF";
volatile uint32_t *wdt = (volatile uint32_t *)0x10007000;
volatile uint32_t *uart_base = (volatile uint32_t *)0x11002000;
volatile uint32_t *uart_reg0 = (volatile uint32_t*)0x0;
volatile uint32_t *uart_reg1 = (volatile uint32_t*)0x0;
void (*send_usb_response)(int, int, int) = (void*)0;
int (*usbdl_put_data)(char* buf, uint32_t sz) = (void*)0;
int (*usbdl_get_data)(char* buf, uint32_t sz) = (void*)0;

void low_uart_put(mcu* param, int ch) {
    while ( !((*param->uart_reg0) & 0x20) )
    {}
    *param->uart_reg1 = ch;
}

void _putchar(mcu* param, char character)
{
    if (character == '\n')
        low_uart_put(param, '\r');
    low_uart_put(param, character);
}

void pdword(mcu* param,uint32_t value)
{
   int i;
   _putchar(param, 0x30);
   _putchar(param, 0x78);
   for (i=3;i>=0;i--){
       _putchar(param, hex[(((value>>(i*8))&0xFF) >>  4) & 0xf]);
       _putchar(param, hex[((value>>(i*8))&0xFF) & 0xf]);
   }
}

uint32_t searchfunc(uint32_t startoffset, uint32_t endoffset, const uint16_t *pattern, uint8_t patternsize) {
    uint8_t matched = 0;
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

uint32_t * ldr_lit(const uint32_t curpc, uint16_t instr, uint8_t *Rt) {
    //#LDR (literal), LDR R1, =SEC_REG
    uint8_t imm8 = instr & 0xFF;
    *Rt = (instr >> 8) & 7;
    uint32_t pc = (((uint32_t)curpc) / 4 * 4);
    return (uint32_t *)(pc + (imm8 * 4) + 4);
}
/*
void ldr_imm(uint16_t instr, uint8_t *simm5, uint8_t *sRt, uint8_t *sRn) {
    *simm5 = (instr >> 6) & 0x1F;
    *sRt = (instr) & 0x7;
    *sRn = (instr >> 3) & 0x7;
}
*/

void send_dword(mcu* param, uint32_t value){
    uint32_t ack=__builtin_bswap32(value);
    #ifdef DEBUG
    print(param,"Send_word\n");
    #endif
    pdword(param, (uint32_t)param->usbdl_put_data);
    print(param,"\n");
    param->usbdl_put_data((char*)&ack, 4);
}

uint32_t recv_dword(mcu* param){
    uint32_t value;
    #ifdef DEBUG
    print(param,"Recv_dword\n");
    #endif
    param->usbdl_get_data((char*)&value,4);
    return __builtin_bswap32(value);
}

uint16_t recv_word(mcu* param){
    uint16_t value;
    #ifdef DEBUG
    print(param,"Recv_word\n");
    #endif
    param->usbdl_get_data((char*)&value,2);
    return __builtin_bswap16(value);
}

void recv_data(mcu* param, char *addr, uint32_t sz, uint32_t flags __attribute__((unused))) {
    for (uint32_t i = 0; i < (((sz + 3) & ~3) / 4); i++) {
        ((uint32_t *)addr)[i] = __builtin_bswap32(recv_dword(param));
    }
}

int print(mcu* param, char* s){
    char c = s[0];
    int i = 0;
    while(c){
        _putchar(param, c);
        c = s[++i];
    }
    return i;
}

void pbyte(mcu* param,uint8_t value)
{
   _putchar(param, hex[((value&0xFF) >>  4) & 0xf]);
   _putchar(param, hex[(value&0xFF) & 0xf]);
}

void hex_dump(mcu* param, const void* data, size_t size) {
    size_t i, j;
    for (i = 0; i < size; ++i) {
        pbyte(param, ((unsigned char*)data)[i]);
        print(param," ");
        if ((i+1) % 4 == 0 || i+1 == size) {
            print(param," ");
            if ((i+1) % 16 == 0) {
                print(param,"\n");
            } else if (i+1 == size) {
                if ((i+1) % 16 <= 8) {
                    print(param," ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    print(param,"   ");
                }
                print(param,"\n");
            }
        }
    }
}

void searchparams(mcu* param) {
    uint16_t i = 0;
    uint32_t offs1 = 0;
    uint32_t bromstart;
    uint32_t bromend;
    uint32_t startpos;

    // A warm welcome to uart
    static const uint16_t uartb[3] = {0x5F31, 0x4E45, 0x0F93};
    for (i = 0; i < 3; ++i) {
        offs1 = searchfunc(brom_bases[i] + 0x100, brom_bases[i] + 0x14000, uartb, 3);
        if (offs1) {
            param->uart_base = (volatile uint32_t *)(((uint32_t *)(offs1 + 0x8))[0] & 0xFFFFFFFF);
            break;
        }
    }

    param->uart_reg0 = (volatile uint32_t*)((volatile uint32_t)param->uart_base + 0x14);
    param->uart_reg1 = (volatile uint32_t*)param->uart_base;

    bromstart = brom_bases[i] + 0x100;
    bromend = brom_bases[i] + 0x14000;

    // Time to find and set the watchdog before it's game over
    static const uint16_t wdts[3] = {0xF641, 0x1071, 0x6088};
    uint8_t Rt = 0;
    offs1 = 0;
    offs1 = searchfunc(bromstart, bromend, wdts, 3);
    if (offs1) {
        param->wdt = (volatile uint32_t *)(ldr_lit((uint32_t)offs1 - 2, ((uint16_t*)(offs1 - 2))[0], &Rt)[0]);
        param->wdt[0] = 0x22000064;
#ifdef DEBUG
        print(param,"A:WDT\n");
        pdword(param,(uint32_t)param->wdt);
#endif
    }
#ifdef DEBUG
    else {
        print(param,"F:WDT\n");
    }
#endif

    // Let's dance with send_usb_response
    static const uint16_t sur1a[2] = {0xB530, 0x2300};
    static const uint16_t sur1b[3] = {0x2808, 0xD00F, 0x2807};
    static const uint16_t sur2[3] = {0x2400, 0xF04F, 0x5389};
    static const uint16_t sur3[3] = {0x2400, 0x2803, 0xD006};
    offs1 = searchfunc(bromstart, bromend, sur1a, 2);
    if (offs1) {
        startpos = searchfunc(offs1 + 6, offs1 + 12, sur1b, 3);
        if (startpos != offs1 + 6){
            offs1 = 0;
        }
    }
    if (!offs1) {
        offs1 = searchfunc(bromstart, bromend, sur2, 3);
        if (offs1){
            offs1 -= 2;
        } else {
            offs1 = searchfunc(bromstart, bromend, sur3, 3);
            if (offs1){
                offs1 -= 4;
            }
        }
    }
    if (offs1){
        param->send_usb_response = (void *)(offs1 | 1);
    }
#ifdef DEBUG
    if (offs1 == 0x0) {
        print(param,"F:sur\n");
        return;
    }
    else{
        print(param,"A:sur: ");
        pdword(param,(uint32_t)param->send_usb_response);
        print(param,"\n");
    }
#endif

    // usbdl_put_data here we are ...
    static const uint16_t sdd[3] = {0xB510, 0x4A06, 0x68D4};
    uint32_t val=searchfunc(bromstart, bromend, sdd, 3);
    param->usbdl_put_data = (void*)((uint32_t)val | 1);
#ifdef DEBUG
    if ((int)param->usbdl_put_data == 1){
        print(param,"F:upd\n");
        return;
    }
    else{
        print(param,"A:upd: ");
        pdword(param, (uint32_t)param->usbdl_put_data);
        print(param,"\n");
    }
#endif

    // usbdl_get_data is a mess ....
    static const uint16_t rcd2[2] = {0xE92D, 0x47F0};
    startpos = bromstart;
    offs1 = -1;
    while (offs1) {
        offs1 = searchfunc(startpos, bromend, rcd2, 2);
        uint8_t* posc = (uint8_t *)offs1;
        if (((uint8_t)posc[7] == (uint8_t) 0x46) && ((uint8_t)posc[8] == (uint8_t)0x92)){
            param->usbdl_get_data = (void *) ((uint32_t)offs1 | 1);
            break;
        }
        startpos = offs1 + 2;
    }
#ifdef DEBUG
    if (!param->usbdl_get_data){
        print(param,"F:ugd\n");
        return;
    }
    else{
        print(param,"A:ugd: ");
        pdword(param, (uint32_t)param->usbdl_get_data);
        print(param,"\n");
    }
#endif

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
    asm volatile ("mcr p15, 0, %0, c7, c5, 6" :: "r"(r0)); // Flush entire branch target cache
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(r0));
    asm volatile ("bic %0,%0,#0x1800" : "=r"(r0) : "r"(r0)); // I+Z bits
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r"(r0));
}

void apmcu_disable_smp(){
    uint32_t r0=0;
    asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r"(r0));
    asm volatile ("bic %0,%0,#0x40" : "=r"(r0) : "r"(r0)); // SMP bit
    asm volatile ("mcr p15, 0, %0, c1, c0, 1" :: "r"(r0));
}
