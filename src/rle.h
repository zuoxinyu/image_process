#pragma once

#include <stddef.h>
#include <stdint.h>

#include "blk.h"

// run-length encoding item: (zeros, bits), (amplitude)
// e.g. (4, 5),(31) denotes a series `0, 0, 0, 0, 31`,
// the amplitude value `31` takes `8` bits in jpeg std
typedef struct {
    uint8_t zeros : 4;
    uint8_t nbits : 4;
    int16_t amp;
    uint8_t _pad;
} xRLEItem;

typedef xRLEItem *xRLETable;

xRLETable rtb_calloc(size_t size);
void rtb_free(xRLETable);
int run_length(xRLETable tbl, xBlock blk);
void rtb_print(xRLETable rtb, size_t size);
