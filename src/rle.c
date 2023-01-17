#include <stdio.h>
#include <stdlib.h>

#include "rle.h"

const xRLEItem RLE_EOB = {{0, 0}, 0};
const xRLEItem RLE_ZRL = {{15, 0}, 0};

xRLETable rtb_calloc(size_t cap)
{
    xRLETable tbl = calloc(cap, sizeof(xRLEItem) + 2);
    tbl[0] = *(xRLEItem *)&cap;
    tbl[1] = (xRLEItem){0};
    return tbl + 2;
}

void rtb_free(xRLETable tbl) { free(tbl - 2); }

size_t rtb_get_size(xRLETable tbl) { return *(size_t *)&tbl[-1]; }

void rtb_set_size(xRLETable tbl, size_t size) { tbl[-1] = *(xRLEItem *)&size; }

size_t rtb_get_cap(xRLETable tbl) { return *(size_t *)&tbl[-2]; }

void rtb_print(xRLETable rtb, size_t size)
{
    printf("RLE Table [%ld]\n", size);

    for (int i = 0; i < size; i++) {
        printf("[(%d, %d)|%d] ", rtb[i].rs.zeros, rtb[i].rs.nbits, rtb[i].amp);
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

int rtb_parse(xRLETable tbl, xBlock blk)
{
    size_t w = blk_get_width(blk);
    size_t zeros = 0, size = 0, last = w * w - 1;
    int16_t amp;

    while (blk[0][last] == 0) {
        last--;
    }

    for (int i = 1; i <= last; i++) {
        amp = (int16_t)blk[0][i];
        if (amp == 0) {
            zeros++;
        } else {
            while (zeros > 15) {
                tbl[size++] = RLE_ZRL;
                zeros -= 16;
            }
            tbl[size++] = (xRLEItem){{zeros & 0x00FF, calc_msb(amp)}, amp};
            zeros = 0;
        }
    }

    if (last != w * w - 1) {
        tbl[size++] = RLE_EOB;
    }

    rtb_set_size(tbl, size);
    return size;
}
