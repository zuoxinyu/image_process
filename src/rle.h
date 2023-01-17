#ifndef _RLE_H_
#define _RLE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>

#include "blk.h"

// run-length encoding item: `(zeros, nbits), (amplitude)`
// e.g. `(4, 5),(30)` denotes a series `0, 0, 0, 0, 30`,
// the amplitude value `30` takes `5` bits in jpeg std
typedef struct {
    struct {
        uint8_t zeros : 4; // [0 .. 15]
        uint8_t nbits : 4; // [1 .. 10]
    } rs;
    uint8_t : 8;
    int16_t amp;
} xRLEItem;

typedef xRLEItem *xRLETable;

extern const xRLEItem RLE_EOB;
extern const xRLEItem RLE_ZRL;

xRLETable rtb_calloc(size_t cap);
void rtb_free(xRLETable tbl);
size_t rtb_get_size(xRLETable tbl);
void rtb_set_size(xRLETable tbl, size_t size);
size_t rtb_get_cap(xRLETable tbl);
void rtb_free(xRLETable);
// parse a zigzagged block into `tbl`,
// `tbl` would be overwrited from the beginning
int rtb_parse(xRLETable tbl, xBlock blk);
void rtb_print(xRLETable rtb, size_t size);

#ifdef __cplusplus
}
#endif
#endif
