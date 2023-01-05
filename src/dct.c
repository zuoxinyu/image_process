#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fftw3.h>

#include "dct.h"

#define N 8

static inline int min(int x, int y) { return x > y ? y : x; }
// static inline int max(int x, int y) { return x > y ? x : y; }

xBlock blk_calloc(int dimX, int dimY)
{
    float **m = calloc(dimX, sizeof(float *));
    float *p = calloc(dimX * dimY, sizeof(float));
    for (int i = 0; i < dimX; i++) {
        m[i] = &p[i * dimY];
    }
    return m;
}

xBlock blk_copy(xBlock blk, int dimX, int dimY)
{
    xBlock copy = blk_calloc(dimX, dimY);
    for (int i = 0; i < dimX * dimY; i++) {
        copy[0][i] = blk[0][i];
    }
    return copy;
}

void blk_free(xBlock blk)
{
    free(blk[0]);
    free(blk);
}

void blk_foreach(xBlock blk, imBlkIterFn iter_func, void *payload)
{
    if (iter_func == NULL)
        return;

    struct {
        xBlock blk;
        void *payload;
    } inner_payload = {blk, payload};

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            blk[i][j] = iter_func(blk[i][j], i, j, &inner_payload);
        }
    }
}

void blk_print(const char *name, xBlock blk, int n, int idx)
{
    printf("%s blk [%d][%d] idx=%d \n", name, n, n, idx);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%+3.8f\t", blk[i][j]);
        }
        printf("\n");
    }
}

xMat mat_calloc(size_t size) { return calloc(sizeof(xReal), size); }

void mat_free(xMat mat) { free(mat); }

xMat mat_copy(xMat mat, size_t size)
{
    xMat copy = mat_calloc(size);
    memcpy(copy, mat, sizeof(xReal) * size);
    return copy;
}

void mat_foreach_blk(xMat pixels, int w, int h, imMatIterFn iter_func,
                     void *payload)
{
    if (iter_func == NULL)
        return;

    struct {
        xMat mat;
        void *payload;
    } inner_payload = {pixels, payload};

    xBlock blk = blk_calloc(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        mat_get_blk(pixels, blk, i, w, h);
        iter_func(blk, i, &inner_payload);
        mat_set_blk(pixels, blk, i, w, h);
    }

    blk_free(blk);
}

void mat_get_blk(const xMat pixels, xBlock blk, int idx, int width, int height)
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

void mat_set_blk(xMat pixels, xBlock blk, int n, int width, int height)
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

#ifdef USE_FFTW3
void dct(xBlock dct_blk, xBlock blk, int dimX, int dimY)
{
    fftwf_plan plan =
        fftwf_plan_r2r_2d(dimX, dimY, blk[0], dct_blk[0], FFTW_REDFT10,
                          FFTW_REDFT10, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

void idct(xBlock dct_blk, xBlock blk, int dimX, int dimY)
{
    fftwf_plan plan =
        fftwf_plan_r2r_2d(dimX, dimY, blk[0], dct_blk[0], FFTW_REDFT01,
                          FFTW_REDFT01, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

#else
void dct(xBlock DCTMatrix, xBlock Matrix, int dimX, int dimY)
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

void idct(xBlock Matrix, xBlock DCTMatrix, int dimX, int dimY)
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

// static float normalize(float x, void *_payload) { return x; }
// static float rescale(float x, void *_payload) { return x / (4 * N * N); }

// the result won't normalize values, e.g. values may (likely) larger 255.0 or
// negative to visualize, perform normalize for each block
void mat_dct_blks(xMat pixels, size_t w, size_t h)
{
    xBlock blk = blk_calloc(N, N), dct_blk = blk_calloc(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        mat_get_blk(pixels, blk, i, w, h);
        dct(dct_blk, blk, N, N);
        mat_set_blk(pixels, dct_blk, i, w, h);
    }

    blk_free(blk);
    blk_free(dct_blk);
}

// the result is scaled by 4*N*N, N is block width/height
// to visualize, perform normalize(value/(4*N*N)) for each block
void mat_idct_blks(xMat pixels, size_t w, size_t h)
{
    xBlock blk = blk_calloc(N, N), idct_blk = blk_calloc(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        mat_get_blk(pixels, blk, i, w, h);
        idct(idct_blk, blk, N, N);
        mat_set_blk(pixels, idct_blk, i, w, h);
    }

    blk_free(blk);
    blk_free(idct_blk);
}
