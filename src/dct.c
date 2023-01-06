#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fftw3.h>

#include "dct.h"

#define N 8

static inline int min(int x, int y) { return x > y ? y : x; }
// static inline int max(int x, int y) { return x > y ? x : y; } // NOLINT

xBlock blk_calloc(size_t dimX, size_t dimY)
{
    float **m = calloc(dimX, sizeof(float *));
    float *data = calloc(dimX * dimY + 2, sizeof(float));
    float *head = data + 2;

    data[0] = (float)dimX;
    data[1] = (float)dimY;

    for (int i = 0; i < dimX; i++) {
        m[i] = &head[i * dimY];
    }
    return m;
}

xBlock blk_copy(xBlock blk)
{
    size_t w = blk_get_width(blk);
    size_t h = blk_get_height(blk);

    xBlock copy = blk_calloc(w, h);
    memcpy(copy[0], blk[0], w * h);
    return copy;
}

void blk_free(xBlock blk)
{
    free(blk[0] - 2);
    free(blk);
}

size_t blk_get_width(xBlock blk) { return blk[0][-2]; }

size_t blk_get_height(xBlock blk) { return blk[0][-1]; }

void blk_foreach(xBlock blk, imBlkIterFn iter_func, void *payload)
{
    if (iter_func == NULL)
        return;

    int w = blk_get_width(blk);
    int h = blk_get_height(blk);

    struct {
        xBlock blk;
        void *payload;
    } inner_payload = {blk, payload};

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            blk[i][j] = iter_func(blk[i][j], i, j, &inner_payload);
        }
    }
}

void blk_print(const char *name, xBlock blk, int idx)
{
    int w = blk_get_width(blk);
    int h = blk_get_height(blk);

    printf("%s blk [%d][%d] idx=%d \n", name, w, w, idx);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            printf("%+3.8f\t", blk[i][j]);
        }
        printf("\n");
    }
}

xMat mat_calloc(size_t w, size_t h)
{
    xReal *data = calloc(sizeof(xReal), w * h + 2);
    data[0] = w;
    data[1] = h;
    return data + 2;
}

void mat_free(xMat mat) { free(mat - 2); }

xMat mat_copy(xMat mat)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    xMat copy = mat_calloc(w, h);
    memcpy(copy, mat, sizeof(xReal) * w * h);
    return copy;
}

size_t mat_get_width(xMat mat) { return (size_t)mat[-2]; }

size_t mat_get_height(xMat mat) { return (size_t)mat[-1]; }

void mat_foreach_blk(xMat mat, imMatIterFn iter_func, void *payload)
{
    if (iter_func == NULL)
        return;

    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    struct {
        xMat mat;
        void *payload;
    } inner_payload = {mat, payload};

    xBlock blk = blk_calloc(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        mat_get_blk(mat, blk, i);
        iter_func(blk, i, &inner_payload);
        mat_set_blk(mat, blk, i);
    }

    blk_free(blk);
}

void mat_get_blk(const xMat mat, xBlock blk, int idx)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);

    int rows = min(h - idx / (w / N) * N, N);
    int cols = min(w - idx % (w / N) * N, N);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            blk[i][j] =
                mat[(idx / (w / N) * N + i) * w + (idx % (w / N) * N + j)];
        }
    }
}

void mat_set_blk(xMat mat, xBlock blk, int idx)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    int rows = min(h - idx / (w / N) * N, N);
    int cols = min(w - idx % (w / N) * N, N);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat[(idx / (w / N) * N + i) * w + (idx % (w / N) * N + j)] =
                blk[i][j];
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
void mat_dct_blks(xMat mat)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    xBlock blk = blk_calloc(N, N), dct_blk = blk_calloc(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        mat_get_blk(mat, blk, i);
        dct(dct_blk, blk, N, N);
        mat_set_blk(mat, dct_blk, i);
    }

    blk_free(blk);
    blk_free(dct_blk);
}

// the result is scaled by 4*N*N, N is block width/height
// to visualize, perform normalize(value/(4*N*N)) for each block
void mat_idct_blks(xMat mat)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    xBlock blk = blk_calloc(N, N), idct_blk = blk_calloc(N, N);

    for (int i = 0; i < w * h / (N * N); i++) {
        mat_get_blk(mat, blk, i);
        idct(idct_blk, blk, N, N);
        mat_set_blk(mat, idct_blk, i);
    }

    blk_free(blk);
    blk_free(idct_blk);
}
