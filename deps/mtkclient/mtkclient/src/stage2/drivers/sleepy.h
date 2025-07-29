#ifndef SLEEPY
#define SLEEPY

#define ITERS_PER_USEC 0x80
unsigned long usleep(unsigned long useconds);
void mdelay (unsigned long msec);
void udelay (unsigned long usec);

#endif