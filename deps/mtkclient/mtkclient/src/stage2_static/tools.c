#include <stdint.h>
#include <stddef.h>
#include "printf.h"
#include "tools.h"
// (c) 2021 by bkerler


static const uint32_t brom_bases[3] = {0, 0x00400000, 0x48000000};
static const char hex[] = "0123456789ABCDEF";
volatile uint32_t *wdt = (volatile uint32_t *)0x10007000;
volatile uint32_t *uart_base = (volatile uint32_t *)0x11002000;
volatile uint32_t *uart_reg0 = (volatile uint32_t*)0x0;
volatile uint32_t *uart_reg1 = (volatile uint32_t*)0x0;
void (*send_usb_response)(int, int, int) = (void*)0x0;
int (*usbdl_put_data)() = (void*)0x0;
int (*usbdl_get_data)() = (void*)0x0;

void low_uart_put(int ch) {
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

void ldr_imm(uint16_t instr, uint8_t *simm5, uint8_t *sRt, uint8_t *sRn) {
    *simm5 = (instr >> 6) & 0x1F;
    *sRt = (instr) & 0x7;
    *sRn = (instr >> 3) & 0x7;
}


void send_dword(uint32_t value){
    uint32_t ack=__builtin_bswap32(value);
    usbdl_put_data(&ack, 4);
}

uint32_t recv_dword(){
    uint32_t value;
    usbdl_get_data(&value,4);
    return __builtin_bswap32(value);
}

uint16_t recv_word(){
    uint16_t value;
    usbdl_get_data(&value,2);
    return __builtin_bswap16(value);
}

int print(char* s){
    char c = s[0];
    int i = 0;
    while(c){
        _putchar(c);
        c = s[++i];
    }
    return i;
}


void pdword(uint32_t value)
{
   int i;
   _putchar(0x30);
   _putchar(0x78);
   for (i=3;i>=0;i--){
        _putchar(hex[(((value>>(i*8))&0xFF) >>  4) & 0xf]);
        _putchar(hex[((value>>(i*8))&0xFF) & 0xf]);
   }
}

void searchparams() {
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
            uart_base = (volatile uint32_t *)(((uint32_t *)(offs1 + 0x8))[0] & 0xFFFFFFFF);
            break;
        }
    }

    uart_reg0 = (volatile uint32_t*)((volatile uint32_t)uart_base + 0x14);
    uart_reg1 = (volatile uint32_t*)uart_base;

    bromstart = brom_bases[i] + 0x100;
    bromend = brom_bases[i] + 0x14000;

    // Time to find and set the watchdog before it's game over
    static const uint16_t wdts[3] = {0xF641, 0x1071, 0x6088};
    uint8_t Rt = 0;
    offs1 = 0;
    offs1 = searchfunc(bromstart, bromend, wdts, 3);
    if (offs1) {
        wdt = (volatile uint32_t *)(ldr_lit((uint32_t)offs1 - 2, ((uint16_t*)(offs1 - 2))[0], &Rt)[0]);
        wdt[0] = 0x22000064;
#ifdef DEBUG
        print("A:WDT\n");
        hex_dump((void*)&wdt, 4);
#endif
    }
#ifdef DEBUG
    else {
        print("F:WDT\n");
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
        send_usb_response = (void *)(offs1 | 1);
    }
#ifdef DEBUG
    if (offs1 == 0x0) {
        print("F:sur\n");
        return 0;
    }
    else{
        print("A:sur\n");
        hex_dump(&send_usb_response, 4);
    }
#endif

    // usbdl_put_data here we are ...
    static const uint16_t sdd[3] = {0xB510, 0x4A06, 0x68D4};
    usbdl_put_data = (void*)(searchfunc(bromstart, bromend, sdd, 3) | 1);
#ifdef DEBUG
    if ((int)usbdl_put_data == 1){
        print("F:upd\n");
        return 0;
    }
    else{
        print("A:upd\n");
        hex_dump(&usbdl_put_data, 4);
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
            usbdl_get_data = (void *) ((uint32_t)offs1 | 1);
            break;
        }
        startpos = offs1 + 2;
    }
#ifdef DEBUG
    if (!usbdl_get_data){
        print("F:ugd\n");
        return 0;
    }
    else{
        print("A:ugd\n");
        hex_dump(&usbdl_get_data, 4);
    }
#endif
}
