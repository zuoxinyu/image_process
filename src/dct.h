#pragma once

#include "blk.h"

void mat_dct_blks(xMat mat, int dim);
void mat_idct_blks(xMat mat, int dim);

// result should be normalize by user-self
void dct(xBlock out, xBlock in, int w, int h);
// inverser transform in fftw3 is scaled by 2N^2, eg, 4*N*N
void idct(xBlock out, xBlock in, int w, int h);
