#include <stdio.h>

typedef word_t word_t;

word_t src[8], dst[8];

word_t ncopy(word_t *src, word_t *dst, word_t len)
{
    word_t count = 0;
    word_t val;
    word_t *src2 = src + 1;
    word_t *dst2 = dst + 1;

    for (int i = 0; i < len; i += 2)
    {
        val = *src;
        *dst = val;
        if (val > 0) count++;
        val = *src2;
        *dst2 = val;
        if (val > 0) count++;
        dst += 2;
        dst2 += 2;
    }
    
    for (; i < len; i++) {
        val = *src
        *dst = val;
        if (val > 0) count++;
        dst++;
        src++;
    }
}

