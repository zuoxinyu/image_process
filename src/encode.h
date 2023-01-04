#pragma once

#include <stddef.h>
#include <stdint.h>

/*
 * 1. rgb split to yuv plannar
 * 2. yuv subsampling to yuv420p
 * 3. split to 8x8 blocks for each plannar
 * 4. DCT
 * 5. run-length compression
 * 6. entropy
 */

float **calloc_blk(int dimX, int dimY);
void free_blk(float **blk);
void print_blk(const char *name, float **blk, int n, int idx);
void get_blk(const float *pixels, float **blk, int idx, int width, int height);
void set_blk(float *pixels, float **blk, int n, int width, int height);
void foreach_blk(float *pixels, int w, int h, float (*iter_func)(float **));
void foreach_blk_elem(float **blk, float (*iter_func)(float));

void dct_blks(float *pixels, size_t w, size_t h);
void idct_blks(float *pixels, size_t w, size_t h);

void dct(float **DCTMatrix, float **Matrix, int n, int m);
void idct(float **Matrix, float **DCTMatrix, int n, int m);
