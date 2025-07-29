#ifndef SEARCHFUNC
#define SEARCHFUNC
uint32_t searchfunc(uint32_t startoffset, uint32_t endoffset, const uint16_t *pattern, uint8_t patternsize);
uint32_t * ldr_lit(const uint32_t curpc, uint16_t instr, uint8_t *Rt);
void ldr_imm(uint16_t instr, uint8_t *simm5, uint8_t *sRt, uint8_t *sRn);
void send_dword(uint32_t value);
uint32_t recv_dword();
uint16_t recv_word();
int print(char* s);
void pdword(uint32_t value);
volatile uint32_t *wdt;
volatile uint32_t *uart_base;
volatile uint32_t *uart_reg0;
volatile uint32_t *uart_reg1;
void (*send_usb_response)(int, int, int);
int (*usbdl_put_data)();
int (*usbdl_get_data)();

void searchparams();

#endif
