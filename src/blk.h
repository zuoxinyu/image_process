#ifndef _BLK_H_
#define _BLK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// float for now
typedef float xReal;
// an 8x8 2d hybrid `xReal` array
typedef xReal *const *xBlock;
// a `sqrt(size)*sqrt(size)` continuous `xReal` array
typedef xReal *xMat;

// TODO: define callback payload and return value
typedef xReal (*xBlkIterFn)(xBlock, xReal, int i, int j, void *);
typedef xBlock (*xMatIterFn)(xMat, xBlock, int i, void *);

xBlock blk_calloc(size_t w, size_t h);
xBlock blk_copy(xBlock blk);
void blk_free(xBlock blk);
size_t blk_get_width(xBlock blk);
size_t blk_get_height(xBlock blk);
void blk_print(const char *name, xBlock blk, int idx);

// outplace mathematical operators
void blk_add(xBlock a, xBlock b, xBlock c);
void blk_diff(xBlock a, xBlock b, xBlock c);
void blk_product(xBlock a, xBlock b, xBlock c);
void blk_devide(xBlock a, xBlock b, xBlock c);
void blk_add_n(xBlock in, xReal n, xBlock out);
void blk_product_n(xBlock in, xReal n, xBlock out);
void blk_clear(xBlock blk, xReal n);
void blk_zigzag(xBlock in, xBlock out);
// TODO: convolution

// inplace iteration, poor man's closure
void blk_foreachi(xBlock blk, xBlkIterFn iter_func, void *payload);
// outplace iteration
void blk_foreacho(xBlock in, xBlock out, xBlkIterFn iter_func, void *payload);

// TODO: deprecate
xMat mat_calloc(size_t w, size_t h);
void mat_free(xMat mat);
xMat mat_copy(xMat mat);
size_t mat_get_width(xMat mat);
size_t mat_get_height(xMat mat);
void mat_get_blk(const xMat mat, xBlock blk, int idx);
void mat_set_blk(xMat mat, xBlock blk, int idx);
// inplace iteration, poor man's closure
void mat_foreach_blk(xMat mat, int dim, xMatIterFn, void *payload);
// outplace mathematical operators
void mat_add(xMat a, xMat b, xMat c);
void mat_diff(xMat a, xMat b, xMat c);
void mat_product(xMat a, xMat b, xMat c);
void mat_devide(xMat a, xMat b, xMat c);
void mat_add_n(xMat in, xReal n, xMat out);
void mat_product_n(xMat in, xReal n, xMat out);

#ifdef __cplusplus
}
#endif
#endif
