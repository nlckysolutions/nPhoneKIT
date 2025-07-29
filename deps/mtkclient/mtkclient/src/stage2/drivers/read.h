#pragma once

#include "types.h"

static inline u16 __raw_readw(const volatile void *addr)
{
    u16 val;
    asm volatile("ldrh %0, %1"
             : "=r" (val)
             : "Q" (*(volatile u16 *)addr));
    return val;
}

static inline u8 __raw_readb(const volatile void *addr)
{
    u8 val;
    asm volatile("ldrb %0, %1"
             : "=r" (val)
             : "Qo" (*(volatile u8 *)addr));
    return val;
}

static inline u32 __raw_readl(const volatile void *addr)
{
    u32 val;
    asm volatile("ldr %0, %1"
             : "=r" (val)
             : "Qo" (*(volatile u32 *)addr));
    return val;
}
