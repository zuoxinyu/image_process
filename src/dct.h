#ifndef _DCT_H_
#define _DCT_H_

#include "blk.h"

#ifdef __cplusplus
extern "C" {
#endif

void mat_dct_blks(xMat mat, int dim);
void mat_idct_blks(xMat mat, int dim);

// result should be normalize by user-self
void dct(xBlock out, xBlock in, int w, int h);
// inverser transform in fftw3 is scaled by 2N^2, eg, 4*N*N
void idct(xBlock out, xBlock in, int w, int h);

void convolution(xBlock out, xBlock kernel);

#ifdef _cplusplus
}
#endif
#endif
