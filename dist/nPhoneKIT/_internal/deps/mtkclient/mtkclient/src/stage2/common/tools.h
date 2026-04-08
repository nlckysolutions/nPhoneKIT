#ifndef TOOLS
#define TOOLS
#include <stdint.h>
#include <stddef.h>
uint32_t searchfunc(uint32_t startoffset, uint32_t endoffset, const uint16_t *pattern, uint8_t patternsize);
uint32_t * ldr_lit(const uint32_t curpc, uint16_t instr, uint8_t *Rt);
void ldr_imm(uint16_t instr, uint8_t *simm5, uint8_t *sRt, uint8_t *sRn);

typedef struct {
    volatile uint32_t *wdt;
    volatile uint32_t *uart_base;
    volatile uint32_t *uart_reg0;
    volatile uint32_t *uart_reg1;
    void (*send_usb_response)(int, int, int);
    int (*usbdl_put_data)();
    int (*usbdl_get_data)();
} mcu;

int print(mcu* param, char* s);
void pdword(mcu* param,uint32_t value);
void send_dword(mcu* param, uint32_t value);
uint32_t recv_dword(mcu* param);
uint16_t recv_word(mcu* param);
void recv_data(mcu* param, char *addr, uint32_t sz, uint32_t flags __attribute__((unused)));


void apmcu_icache_invalidate();
void apmcu_isb();
void apmcu_disable_icache();
void apmcu_disable_smp();

void hex_dump(mcu* param, const void* data, size_t size);

void searchparams();
#endif