#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/blk.h"
#include "src/dct.h"
#include "src/ppm.h"

#define N 8

#if N > 16
#define blk_print(name, b, idx)
#endif

enum TRANS_KIND {
    DCT,
    IDCT,
    DFT,
    IDFT,
};

enum FORMULA_ID {
    FORMULA_SIN,
    FORMULA_COS,
};

static inline xReal min(xReal x, xReal y) { return x > y ? y : x; }
static inline xReal max(xReal x, xReal y) { return x > y ? x : y; }

static xReal normalize(xBlock blk, xReal x, int i, int j, void *payload)
{
    xReal *minmax = payload;
    xReal minv = minmax[0], maxv = minmax[1];

    return 255. * (x - minv) / (maxv - minv);
}

static xReal rescale(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return x / (4. * N * N);
}

void calculate_min_max(xBlock blk, int w, int h, xReal *minmax)
{
    minmax[0] = blk[0][0];
    minmax[1] = blk[0][0];

    for (int i = 0; i < w * h; i++) {
        minmax[0] = min(minmax[0], blk[0][i]);
        minmax[1] = max(minmax[1], blk[0][i]);
    }
}

void gen_blk(int formula, xBlock blk, int w, int h)
{
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            blk[i][j] = (sinf(j * w / (2 * M_PI)) + 1) * 128;
            blk[j][i] = (sinf(i * h / (2 * M_PI)) + 1) * 128;
        }
    }
}

void blk2uint8(xBlock blk, uint8_t *buf, int w, int h)
{
    for (int i = 0; i < w * h; i++) {
        buf[i] = (uint8_t)blk[0][i];
    }
}

int write_file(PPM *ppm, xBlock blk, const char *name)
{
    blk2uint8(blk, ppm->data, ppm->width, ppm->height);
    return ppm_write_file(ppm, name);
}

int gen_base_blks(int n)
{
    xMat mat = mat_calloc(n * n, n * n);
    uint8_t *buf = malloc(sizeof(uint8_t) * n * n * n * n);
    PPM *ppm = ppm_create(PPM_FMT_P5, n * n, n * n, buf);
    xBlock blk = blk_calloc(n, n);
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            blk_clear(blk, 0);
            blk[i][j] = 255;
            idct(blk, blk, n, n);
            mat_set_blk(mat, blk, i * n + j);
        }
    }

    blk2uint8(&mat, buf, n * n, n * n);
    write_file(ppm, &mat, "base.pgm");

    mat_free(mat);
    blk_free(blk);
    ppm_free(ppm);

    return 0;
}

int main(int argc, char *argv[])
{
    uint8_t *buf = malloc(sizeof(uint8_t) * N * N);
    xBlock blk_idct = blk_calloc(N, N);
    xBlock blk_dct = blk_copy(blk_idct);
    PPM *ppm = ppm_create(PPM_FMT_P5, N, N, buf);
    ppm_dump_header(ppm);

    gen_blk(FORMULA_SIN, blk_idct, N, N);
    blk_clear(blk_idct, 3);
    blk_print("sin", blk_idct, 0);
    write_file(ppm, blk_idct, "sin.pgm");

    dct(blk_dct, blk_idct, N, N);
    blk_print("dct", blk_dct, 0);
    idct(blk_idct, blk_dct, N, N);
    blk_print("idct", blk_idct, 0);

    xReal minmax[2];
    calculate_min_max(blk_dct, N, N, minmax);
    blk_foreachi(blk_dct, normalize, &minmax);
    blk_print("normalized dct", blk_dct, 0);
    write_file(ppm, blk_dct, "sin.dct.pgm");

    blk_foreachi(blk_idct, rescale, NULL);
    blk_print("rescaled idct", blk_idct, 0);
    write_file(ppm, blk_idct, "sin.idct.pgm");

    blk_clear(blk_idct, 0);
    blk_idct[5][0] = 255;
    blk_idct[0][5] = 255;
    blk_print("test idct", blk_idct, 0);
    write_file(ppm, blk_idct, "test.idct.pgm");

    idct(blk_idct, blk_idct, N, N);
    blk_foreachi(blk_idct, rescale, NULL);
    calculate_min_max(blk_idct, N, N, minmax);
    blk_foreachi(blk_idct, normalize, &minmax);
    blk_print("test dct", blk_idct, 0);
    write_file(ppm, blk_idct, "test.dct.pgm");

    // gen_base_blks(8);

    ppm_free(ppm);
    blk_free(blk_dct);
    blk_free(blk_idct);
    free(buf);

    printf("bye!\n");
}
