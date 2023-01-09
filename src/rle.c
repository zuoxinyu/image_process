#include <stdio.h>
#include <stdlib.h>

#include "rle.h"

xRLETable rtb_calloc(size_t cap) { return calloc(sizeof(xRLEItem), cap); }

void rtb_free(xRLETable tbl) { free(tbl); }

void rtb_print(xRLETable rtb, size_t size)
{
    printf("RLE Table [%ld]\n", size);

    for (int i = 0; i < size; i++) {
        printf("[(%d, %d)|%d] ", rtb[i].zeros, rtb[i].nbits, rtb[i].amp);
    }

    printf("\n");
}

static size_t calc_msb(int16_t n)
{
    if (n < 0)
        n *= -1;
#if __GNUC__
    return n == 0 ? 0 : 16 - __builtin_clz(n);
#else
    int nbits = 0;
    while (n >>= 1)
        nbits++;
    return nbits;
#endif
}

int run_length(xRLETable tbl, xBlock blk)
{
    int w = blk_get_width(blk);
    int zeros = 0, j = 0;
    int16_t num, last = w * w - 1;

    while (blk[0][last] == 0) {
        last--;
    }

    for (int i = 1; i <= last; i++) {
        num = (int16_t)blk[0][i];
        if (zeros >= 16) {
            // 16 zeros
            tbl[j].zeros = 15;
            tbl[j].nbits = 0;
            tbl[j].amp = 0;
            zeros = 0;
            j++;
        } else if (num == 0) {
            zeros++;
        } else {
            tbl[j].zeros = zeros;
            tbl[j].nbits = calc_msb(num);
            tbl[j].amp = num;
            zeros = 0;
            j++;
        }
    }

    if (last != w * w - 1) {
        // EOB
        tbl[j].zeros = 0;
        tbl[j].nbits = 0;
        tbl[j].amp = 0;
    }

    return j + 1;
}
