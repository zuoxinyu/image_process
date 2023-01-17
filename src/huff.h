#ifndef _HUFF_H_
#define _HUFF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "rle.h"

/** JPEG standard Huffman tables */
/** Luminance (Y) - DC */
extern const uint8_t jpec_dc_nodes[17];
extern const int jpec_dc_nb_vals;
extern const uint8_t jpec_dc_vals[12];
/** Luminance (Y) - AC */
extern const uint8_t jpec_ac_nodes[17];
extern const int jpec_ac_nb_vals;
extern const uint8_t jpec_ac_vals[162];

/** Huffman inverted tables */
/** Luminance (Y) - DC */
extern const uint8_t jpec_dc_len[12];
extern const int jpec_dc_code[12];
/** Luminance (Y) - AC */
extern const int8_t jpec_ac_len[256];
extern const int jpec_ac_code[256];

// encode a run-length encoded table into buffer
int huff_encode_tbl(uint32_t *bitbuf, xRLETable tbl);
// encode `nbits` of bits into buffer
int huff_encode_bits(uint32_t *bitbuf, uint16_t nbits, uint16_t bits);

#ifdef __cplusplus
}
#endif
#endif
