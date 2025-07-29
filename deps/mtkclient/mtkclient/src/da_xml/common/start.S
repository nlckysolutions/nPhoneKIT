.syntax unified

.code 32

.global start
.section .text.start
start:
    #add r3, pc, #1
    #bx r3

.global apmcu_dcache_clean_invalidate
.section .text
.type    apmcu_dcache_clean_invalidate,%function
apmcu_dcache_clean_invalidate:
    push    {r4,r5,r7,r9,r10,r11}
    dmb                                     /* ensure ordering with previous memory accesses */
    mrc     p15, 1, r0, c0, c0, 1           /* read clidr */
    ands    r3, r0, #0x7000000              /* extract loc from clidr */
    mov     r3, r3, lsr #23                 /* left align loc bit field */
    beq     ci_finished                     /* if loc is 0, then no need to clean */
    mov     r10, #0                         /* start clean at cache level 0 */
ci_loop1:
    add     r2, r10, r10, lsr #1            /* work out 3x current cache level */
    mov     r1, r0, lsr r2                  /* extract cache type bits from clidr */
    and     r1, r1, #7                      /* mask of the bits for current cache only */
    cmp     r1, #2                          /* see what cache we have at this level */
    blt     ci_skip                         /* skip if no cache, or just i-cache */
    mcr     p15, 2, r10, c0, c0, 0          /* select current cache level in cssr */
    isb                                     /* isb to sych the new cssr&csidr */
    mrc     p15, 1, r1, c0, c0, 0           /* read the new csidr */
    and     r2, r1, #7                      /* extract the length of the cache lines */
    add     r2, r2, #4                      /* add 4 (line length offset) */
    ldr     r4, =0x3ff
    ands    r4, r4, r1, lsr #3              /* find maximum number on the way size */
    clz     r5, r4                          /* find bit position of way size increment */
    ldr     r7, =0x7fff
    ands    r7, r7, r1, lsr #13             /* extract max number of the index size */
ci_loop2:
    mov     r9, r4                          /* create working copy of max way size */
ci_loop3:
    orr     r11, r10, r9, lsl r5            /* factor way and cache number into r11 */
    orr     r11, r11, r7, lsl r2            /* factor index number into r11 */
    mcr     p15, 0, r11, c7, c14, 2         /* clean & invalidate by set/way */
    subs    r9, r9, #1                      /* decrement the way */
    bge     ci_loop3
    subs    r7, r7, #1                      /* decrement the index */
    bge     ci_loop2
ci_skip:
    add     r10, r10, #2                    /* increment cache number */
    cmp     r3, r10
    bgt     ci_loop1
ci_finished:
    mov     r10, #0                         /* swith back to cache level 0 */
    mcr     p15, 2, r10, c0, c0, 0          /* select current cache level in cssr */
    dsb
    isb
    pop     {r4,r5,r7,r9,r10,r11}
    bx      lr


.global  apmcu_dcache_invalidate
.section .text
.type    apmcu_dcache_invalidate,%function
apmcu_dcache_invalidate:
    push    {r4,r5,r7,r9,r10,r11}
    dmb                                     /* ensure ordering with previous memory accesses */
    mrc     p15, 1, r0, c0, c0, 1           /* read clidr */
    ands    r3, r0, #0x7000000              /* extract loc from clidr */
    mov     r3, r3, lsr #23                 /* left align loc bit field */
    beq     cii_finished                     /* if loc is 0, then no need to clean */
    mov     r10, #0                         /* start clean at cache level 0 */
cii_loop1:
    add     r2, r10, r10, lsr #1            /* work out 3x current cache level */
    mov     r1, r0, lsr r2                  /* extract cache type bits from clidr */
    and     r1, r1, #7                      /* mask of the bits for current cache only */
    cmp     r1, #2                          /* see what cache we have at this level */
    blt     cii_skip                         /* skip if no cache, or just i-cache */
    mcr     p15, 2, r10, c0, c0, 0          /* select current cache level in cssr */
    isb                                     /* isb to sych the new cssr&csidr */
    mrc     p15, 1, r1, c0, c0, 0           /* read the new csidr */
    and     r2, r1, #7                      /* extract the length of the cache lines */
    add     r2, r2, #4                      /* add 4 (line length offset) */
    ldr     r4, =0x3ff
    ands    r4, r4, r1, lsr #3              /* find maximum number on the way size */
    clz     r5, r4                          /* find bit position of way size increment */
    ldr     r7, =0x7fff
    ands    r7, r7, r1, lsr #13             /* extract max number of the index size */
cii_loop2:
    mov     r9, r4                          /* create working copy of max way size */
cii_loop3:
    orr     r11, r10, r9, lsl r5            /* factor way and cache number into r11 */
    orr     r11, r11, r7, lsl r2            /* factor index number into r11 */
    mcr     p15, 0, r11, c7, c6, 2         /* invalidate by set/way */
    subs    r9, r9, #1                      /* decrement the way */
    bge     cii_loop3
    subs    r7, r7, #1                      /* decrement the index */
    bge     cii_loop2
cii_skip:
    add     r10, r10, #2                    /* increment cache number */
    cmp     r3, r10
    bgt     cii_loop1
cii_finished:
    mov     r10, #0                         /* swith back to cache level 0 */
    mcr     p15, 2, r10, c0, c0, 0          /* select current cache level in cssr */
    dsb
    isb
    pop     {r4,r5,r7,r9,r10,r11}
    bx      lr


.global cache_init
.section .text
.type    cache_init,%function
cache_init:
    PUSH            {R4-R11,LR}
    MOV             R7, R0
    MRS             R12, CPSR
    CPSID           AIF
    TST             R7, #2
    BEQ             mmt
    MRC             p15, 0, R0,c1,c0, 0
    TST             R0, #4
    BEQ             mma
    BIC             R0, R0, #4
    MCR             p15, 0, R0,c1,c0, 0
    BL              apmcu_dcache_clean_invalidate
    B               mmt
mma:
    BL              apmcu_dcache_invalidate
mmt:
    TST             R7, #1
    BEQ             mml
    MRC             p15, 0, R0,c1,c0, 0
    BIC             R0, R0, #0x1000
    MCR             p15, 0, R0,c1,c0, 0
mml:
    MOV             R0, #0
    MCR             p15, 0, R0,c7,c5, 0
    MSR             CPSR_cf, R12
    POP             {R4-R11,PC}


.global cache_close
.section .text
.type    cache_close,%function
cache_close:
    PUSH            {R4-R11,LR}
    MOV             R7, R0
    MRS             R12, CPSR
    CPSID           AIF
    TST             R7, #2
    BEQ             cci
    MRC             p15, 0, R0,c1,c0, 0
    TST             R0, #4
    BNE             cci
    BL              apmcu_dcache_invalidate
    MRC             p15, 0, R0,c1,c0, 0
    ORR             R0, R0, #4
    MCR             p15, 0, R0,c1,c0, 0
cci:
    TST             R7, #1
    BEQ             cct
    MOV             R0, #0
    MCR             p15, 0, R0,c7,c5, 0
    MRC             p15, 0, R0,c1,c0, 0
    ORR             R0, R0, #0x1000
    MCR             p15, 0, R0,c1,c0, 0
cct:
    MSR             CPSR_cf, R12
    POP             {R4-R11,PC}
