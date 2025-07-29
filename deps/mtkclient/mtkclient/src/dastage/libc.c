#include "libc.h"

#include <stdarg.h>

#define GENDIV_MACRO(name, type)        \
    typedef struct name##_result {      \
        type quo;                       \
        type rem;                       \
    } name##_result_t;                  \
\
    name##_result_t name(               \
        type num,                       \
        type dem                        \
    ) {                                 \
        type tmp = dem;                 \
        name##_result_t ans = {0};      \
        if (dem == 0) {                 \
            return ans;                 \
        }                               \
\
        while (tmp <= num >> 1) {       \
            tmp <<= 1;                  \
        }                               \
\
        do {                            \
            if (num >= tmp) {           \
                num -= tmp;             \
                ans.quo++;              \
            }                           \
            ans.quo <<= 1;              \
            tmp >>= 1;                  \
        } while (tmp >= dem);           \
\
        ans.quo >>= 1;                  \
        ans.rem = num;                  \
        return ans;                     \
    }                                   \
\
    type __aeabi_##name     (type a, type b) {  \
        return name(a, b).quo;                  \
    }                                           \
\
    type __aeabi_##name##mod(type a, type b) {  \
        return name(a, b).rem;                  \
    }

GENDIV_MACRO(uidiv, u32_t)
GENDIV_MACRO(uldiv, u64_t)

void*  memset(void*  dst, int c, u32_t n)
{
    char*  q   = dst;
    char*  end = q + n;

    for (;;) {
        if (q >= end) break;
        *q++ = (char) c;
        if (q >= end) break;
        *q++ = (char) c;
        if (q >= end) break;
        *q++ = (char) c;
        if (q >= end) break;
        *q++ = (char) c;
    }

  return dst;
}

u32_t
strlen(const char *str)
{
    const char *s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}

char *
strcpy(char *to, const char *from)
{
    char *save = to;

    for (; (*to = *from) != '\0'; ++from, ++to);
    return(save);
}

/*
 * Compare strings.
 */
int
strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == 0)
            return (0);
    return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}

int
strncmp(const char *s1, const char *s2, u32_t n)
{
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++)
            return (*(unsigned char *)s1 - *(unsigned char *)--s2);
        if (*s1++ == 0)
            break;
    } while (--n != 0);
    return (0);
}

void *memcpy(void *dest, const void *src, size_t n)
{
    char *dp = dest;
    const char *sp = src;
    while (n--)
        *dp++ = *sp++;
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;
    while(n--)
        if( *p1 != *p2 )
            return *p1 - *p2;
        else
            p1++,p2++;
    return 0;
}

char *strstr(const char *s1, const char *s2)
{
    size_t n = strlen(s2);
    while(*s1)
        if(!memcmp(s1++,s2,n))
            return (char *)s1-1;
    return 0;
}
