#pragma once

#include <stddef.h>
#include <stdint.h>

// float for now
typedef float xReal;
// an 8x8 2d hybrid `xReal` array
typedef xReal **xBlock;
// a `sqrt(size)*sqrt(size)` continuous `xReal` array
typedef xReal *xMat;

// TODO: define callback payload and return value
typedef xReal (*imBlkIterFn)(xReal, int i, int j, void *);
typedef xBlock (*imMatIterFn)(xBlock, int i, void *);

xBlock blk_calloc(size_t w, size_t h);
xBlock blk_copy(xBlock blk);
void blk_free(xBlock blk);
size_t blk_get_width(xBlock blk);
size_t blk_get_height(xBlock blk);
void blk_print(const char *name, xBlock blk, int idx);
// inplace iteration, poor man's closure
void blk_foreach(xBlock blk, imBlkIterFn iter_func, void *payload);

xMat mat_calloc(size_t w, size_t h);
void mat_free(xMat mat);
xMat mat_copy(xMat mat);
size_t mat_get_width(xMat mat);
size_t mat_get_height(xMat mat);
void mat_get_blk(const xMat mat, xBlock blk, int idx);
void mat_set_blk(xMat mat, xBlock blk, int idx);
// inplace iteration, poor man's closure
void mat_foreach_blk(xMat mat, imMatIterFn, void *payload);
void mat_dct_blks(xMat mat);
void mat_idct_blks(xMat mat);

// result should be normalize by user-self
void dct(xBlock out, xBlock in, int w, int h);
// inverser transform in fftw3 is scaled by 2N^2, eg, 4*N*N
void idct(xBlock out, xBlock in, int w, int h);
