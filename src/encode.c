#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <fftw3.h>

#include "encode.h"

#define N 8

static inline int min(int x, int y) { return x > y ? y : x; }
static inline int max(int x, int y) { return x > y ? x : y; }

float **calloc_blk(int dimX, int dimY)
{
    float **m = calloc(dimX, sizeof(float *));
    float *p = calloc(dimX * dimY, sizeof(float));
    int i;
    for (i = 0; i < dimX; i++) {
        m[i] = &p[i * dimY];
    }
    return m;
}

void free_blk(float **blk)
{
    free(blk[0]);
    free(blk);
}

// inplace
void foreach_blk(float *pixels, int w, int h, float (*iter_func)(float **))
{
    float **blk = calloc_blk(N, N), **idct_blk = calloc_blk(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        iter_func(blk);
    }

    free_blk(blk);
    free_blk(idct_blk);
}

void foreach_blk_elem(float **blk, float (*iter_func)(float))
{
    if (iter_func == NULL)
        return;
    for (int i = 0; i < N * N; i++) {
        blk[0][i] = iter_func(blk[0][i]);
    }
}

void print_blk(const char *name, float **blk, int n, int idx)
{
    printf("%s blk [%d][%d] idx=%d \n", name, n, n, idx);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%+3.8f\t", blk[i][j]);
        }
        printf("\n");
    }
}

void get_blk(const float *pixels, float **blk, int idx, int width, int height)
{
    int rows = min(height - idx / (width / N) * N, N);
    int cols = min(width - idx % (width / N) * N, N);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            blk[i][j] = pixels[(idx / (width / N) * N + i) * width +
                               (idx % (width / N) * N + j)];
        }
    }
}

void set_blk(float *pixels, float **blk, int n, int width, int height)
{
    int rows = min(height - n / (width / N) * N, N);
    int cols = min(width - n % (width / N) * N, N);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            pixels[(n / (width / N) * N + i) * width +
                   (n % (width / N) * N + j)] = blk[i][j];
        }
    }
}

#ifndef USE_FFTW3
void dct(float **DCTMatrix, float **Matrix, int dimX, int dimY)
{
    fftwf_plan plan =
        fftwf_plan_r2r_2d(dimX, dimY, Matrix[0], DCTMatrix[0], FFTW_REDFT10,
                          FFTW_REDFT10, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

// inverser transform in fftw3 is scaled by 2N^2, eg, 4*N*N
void idct(float **DCTMatrix, float **Matrix, int dimX, int dimY)
{
    fftwf_plan plan =
        fftwf_plan_r2r_2d(dimX, dimY, Matrix[0], DCTMatrix[0], FFTW_REDFT01,
                          FFTW_REDFT01, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

#else
void dct(float **DCTMatrix, float **Matrix, int dimX, int dimY)
{
    int i, j, u, v;
    for (u = 0; u < dimX; ++u) {
        for (v = 0; v < dimY; ++v) {
            DCTMatrix[u][v] = 0;
            for (i = 0; i < dimX; i++) {
                for (j = 0; j < dimY; j++) {
                    DCTMatrix[u][v] +=
                        Matrix[i][j] *
                        cos(M_PI / ((float)dimX) * (i + 1. / 2.) * u) *
                        cos(M_PI / ((float)dimY) * (j + 1. / 2.) * v);
                }
            }
        }
    }
}

void idct(float **Matrix, float **DCTMatrix, int dimX, int dimY)
{
    int i, j, u, v;

    for (u = 0; u < dimX; ++u) {
        for (v = 0; v < dimY; ++v) {
            Matrix[u][v] = 1 / 4. * DCTMatrix[0][0];
            for (i = 1; i < dimX; i++) {
                Matrix[u][v] += 1 / 2. * DCTMatrix[i][0];
            }
            for (j = 1; j < dimY; j++) {
                Matrix[u][v] += 1 / 2. * DCTMatrix[0][j];
            }

            for (i = 1; i < dimX; i++) {
                for (j = 1; j < dimY; j++) {
                    Matrix[u][v] +=
                        DCTMatrix[i][j] *
                        cos(M_PI / ((float)dimX) * (u + 1. / 2.) * i) *
                        cos(M_PI / ((float)dimY) * (v + 1. / 2.) * j);
                }
            }
            Matrix[u][v] *= 2. / ((float)dimX) * 2. / ((float)dimY);
        }
    }
}

#endif

static float normalize(float x) { return x / (4 * N * N); }

void dct_blks(float *pixels, size_t w, size_t h)
{
    float **blk = calloc_blk(N, N), **dct_blk = calloc_blk(N, N);

    get_blk(pixels, blk, 20, w, h);
    print_blk("raw", blk, N, 20);
    dct(dct_blk, blk, N, N);
    print_blk("dct", dct_blk, N, 20);
    idct(blk, dct_blk, N, N);
    print_blk("idct", blk, N, 20);

    for (int i = 0; i < w * h / (N * N); i++) {
        get_blk(pixels, blk, i, w, h);
        dct(dct_blk, blk, N, N);
        set_blk(pixels, dct_blk, i, w, h);
    }

    free_blk(blk);
    free_blk(dct_blk);
}

void idct_blks(float *pixels, size_t w, size_t h)
{
    float **blk = calloc_blk(N, N), **idct_blk = calloc_blk(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        get_blk(pixels, blk, i, w, h);
        idct(idct_blk, blk, N, N);
        foreach_blk_elem(idct_blk, normalize);
        set_blk(pixels, idct_blk, i, w, h);
    }

    free_blk(blk);
    free_blk(idct_blk);
}
