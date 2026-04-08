#pragma once

#include <inttypes.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef int atomic_t;
typedef int bool;

#include "../printf.h"

#define printk printf
#define KERN_WARNING "[KERN] "

#define N_MSG(evt, fmt, args...) printf(fmt, ##args)
#define ERR_MSG printf
#define IRQ_MSG printf

void fatal(const char *err);


#define barrier() __asm__ __volatile__("": : :"memory")
#define mb()        barrier()

#define mmc_host msdc_host
