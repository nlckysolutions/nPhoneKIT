#include "sleepy.h"

/*unsigned long usleep(unsigned long useconds) {
    	__asm__ __volatile__ (
			" push {r0,r1}		\n\t"
			" mov r0, %[usec]	\n\t"
			" 1: \n\t"
			" mov r1, %[iter] 	\n\t"
			" 2:				\n\t"
			" subs r1, r1, #0x1 \n\t"
			" bne   2b    		\n\t"
			" subs r0,r0,#0x1 	\n\t"
			"  bne 1b 			\n\t"
			" pop {r0,r1} 		\n\t"
			:: [iter] "r" (ITERS_PER_USEC), [usec] "r" (useconds)
	);
	return 0;
}

void mdelay (unsigned long msec)
{
    usleep(1000*msec);
}*/

/* delay usec useconds */
/*void udelay (unsigned long usec)
{
    usleep(usec);
}*/

void sleepy(void) {
    // TODO: do better
    for (volatile int i = 0; i < 0x80000; ++i) {}
}

unsigned long usleep (unsigned long msec)
{
    (void)msec;
    sleepy();
    return 0;
}

void mdelay (unsigned long msec)
{
    (void)msec;
    sleepy();
}

/* delay usec useconds */
void udelay (unsigned long usec)
{
    (void)usec;
    sleepy();
}