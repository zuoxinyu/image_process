#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/dct.h"
#include "src/ppm.h"

#define N 8

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

static xReal normalize(xReal x, int i, int j, void *payload)
{
    struct _Payload {
        xBlock blk;
        void *payload;
    };

    xReal *minmax = ((struct _Payload *)payload)->payload;
    xReal minv = minmax[0], maxv = minmax[1];

    return 255. * (x - minv) / (maxv - minv);
}

static xReal rescale(xReal x, int i, int j, void *_payload)
{
    return x / (4 * N * N);
}

void transform(int kind, uint8_t *buf, int w, int h)
{
    xBlock blk = blk_calloc(w, h);

    for (int i = 0; i < w * h; i++) {
        blk[0][i] = buf[i];
    }

    xBlock dstblk = blk_copy(blk, w, h);
    blk_print("raw", blk, w, h);

    switch (kind) {
    case DCT:
        dct(dstblk, blk, w, h);
        blk_print("dct", dstblk, w, h);
        xReal minmax[2] = {blk[0][0], dstblk[0][0]};

        for (int i = 0; i < w * h; i++) {
            minmax[0] = min(minmax[0], dstblk[0][i]);
        }
        blk_foreach(dstblk, normalize, minmax);
        blk_print("normalized dct", dstblk, w, h);
        break;
    case IDCT:
        idct(dstblk, blk, w, h);
        blk_print("idct", dstblk, w, h);
        blk_foreach(dstblk, rescale, NULL);
        blk_print("rescaled idct", dstblk, w, h);
        break;
    default:
        break;
    }

    for (int i = 0; i < w * h; i++) {
        buf[i] = dstblk[0][i];
    }

    blk_free(blk);
    blk_free(dstblk);
}

void gen_buf(int formula, uint8_t *buf, int w, int h)
{
    xBlock blk = blk_calloc(w, h);
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            blk[i][j] = (sinf((j * w) / (2 * M_PI)) + 1) * 128;
        }
    }

    for (int i = 0; i < w * h; i++) {
        buf[i] = blk[0][i];
    }
    blk_free(blk);
}

int main(int argc, char *argv[])
{
    uint8_t *buf = malloc(sizeof(uint8_t) * N * N * 3);
    PPM *ppm = ppm_create(PPM_FMT_P5, N, N, NULL);
    ppm_dump_header(ppm);

    gen_buf(0, buf, N, N);

    ppm->data = buf;
    ppm_write_file(ppm, "sin.pgm");

    transform(DCT, buf, N, N);
    ppm_write_file(ppm, "sin.dct.pgm");

    transform(IDCT, buf, N, N);
    ppm_write_file(ppm, "sin.idct.pgm");

    free(buf);

    printf("bye!\n");
}

/*
float testBlockA[8][8] = {{255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255},
                          {255, 255, 255, 255, 255, 255, 255, 255}},

      testBlockB[8][8] = {{255, 0, 255, 0, 255, 0, 255, 0},
                          {0, 255, 0, 255, 0, 255, 0, 255},
                          {255, 0, 255, 0, 255, 0, 255, 0},
                          {0, 255, 0, 255, 0, 255, 0, 255},
                          {255, 0, 255, 0, 255, 0, 255, 0},
                          {0, 255, 0, 255, 0, 255, 0, 255},
                          {255, 0, 255, 0, 255, 0, 255, 0},
                          {0, 255, 0, 255, 0, 255, 0, 255}},

      testBlockC[8][8] = {{126, 129, 128, 131, 131, 129, 132, 131},
                          {126, 130, 128, 131, 131, 129, 132, 131},
                          {126, 129, 128, 131, 131, 129, 132, 131},
                          {126, 129, 128, 131, 131, 129, 132, 131},
                          {127, 131, 129, 131, 132, 130, 133, 131},
                          {128, 132, 130, 132, 133, 131, 134, 132},
                          {127, 131, 129, 131, 132, 130, 133, 131},
                          {126, 128, 127, 130, 130, 128, 131, 130}};
*/
