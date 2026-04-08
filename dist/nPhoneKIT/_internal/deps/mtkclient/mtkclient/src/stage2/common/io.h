# define __iomem
# define __force

static inline void __raw_writew(u16 val, volatile void __iomem *addr)
{
    asm volatile("strh %1, %0"
             : "+Q" (*(volatile u16 __force *)addr)
             : "r" (val));
}

static inline u16 __raw_readw(const volatile void __iomem *addr)
{
    u16 val;
    asm volatile("ldrh %1, %0"
             : "+Q" (*(volatile u16 __force *)addr),
               "=r" (val));
    return val;
}

static inline void __raw_writeb(u8 val, volatile void __iomem *addr)
{
    asm volatile("strb %1, %0"
             : "+Qo" (*(volatile u8 __force *)addr)
             : "r" (val));
}

static inline void __raw_writel(u32 val, volatile void __iomem *addr)
{
    asm volatile("str %1, %0"
             : "+Qo" (*(volatile u32 __force *)addr)
             : "r" (val));
}

static inline u8 __raw_readb(const volatile void __iomem *addr)
{
    u8 val;
    asm volatile("ldrb %1, %0"
             : "+Qo" (*(volatile u8 __force *)addr),
               "=r" (val));
    return val;
}

static inline u32 __raw_readl(const volatile void __iomem *addr)
{
    u32 val;
    asm volatile("ldr %1, %0"
             : "+Qo" (*(volatile u32 __force *)addr),
               "=r" (val));
    return val;
}
