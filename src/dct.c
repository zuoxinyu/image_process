#include <math.h>

#include <fftw3.h>

#include "dct.h"

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
void dct(xBlock out, xBlock in, int dimX, int dimY)
{
    int x, y, u, v;
    xReal Cu, Cv;
    for (v = 0; v < dimY; ++v) {
        for (u = 0; u < dimX; ++u) {
            out[v][u] = 0.f;
            Cu = v == 0 ? 1.f / sqrtf(2.f) : 1.f;
            Cv = u == 0 ? 1.f / sqrtf(2.f) : 1.f;

            for (y = 0; y < dimY; y++) {
                for (x = 0; x < dimX; x++) {
                    out[v][u] +=
                        in[y][x] *
                        cosf(M_PI * (2 * y + 1.f) * v / (2 * (xReal)dimY)) *
                        cosf(M_PI * (2 * x + 1.f) * u / (2 * (xReal)dimX));
                }
            }

            out[v][u] *= Cu * Cv * 1.f / 4.f;
        }
    }
}

void idct(xBlock out, xBlock in, int dimX, int dimY)
{
    int x, y, u, v;
    xReal Cu, Cv;

    for (y = 0; y < dimX; ++y) {
        for (x = 0; x < dimY; ++x) {
            out[y][x] = 0.f;

            for (v = 0; v < dimY; v++) {
                for (u = 0; u < dimX; u++) {
                    Cu = v == 0 ? 1.f / sqrtf(2.f) : 1.f;
                    Cv = u == 0 ? 1.f / sqrtf(2.f) : 1.f;
                    out[y][x] +=
                        in[v][u] * Cu * Cv *
                        cosf(M_PI * (2 * y + 1.f) * v / (2 * (xReal)dimY)) *
                        cosf(M_PI * (2 * x + 1.f) * u / (2 * (xReal)dimX));
                }
            }

            out[y][x] *= 1.f / 4.f;
        }
    }
}
#endif

// static xReal normalize(xReal x, void *_payload) { return x; }
// static xReal rescale(xReal x, void *_payload) { return x / (4 * N * N); }

// the result won't normalize values, e.g. values may (likely) larger 255.0 or
// negative to visualize, perform normalize for each block
void mat_dct_blks(xMat mat, int dim)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    xBlock blk = blk_calloc(dim, dim), dct_blk = blk_calloc(dim, dim);

    for (int i = 0; i < w * h / (dim * dim); i++) {
        mat_get_blk(mat, blk, i);
        dct(dct_blk, blk, dim, dim);
        mat_set_blk(mat, dct_blk, i);
    }

    blk_free(blk);
    blk_free(dct_blk);
}

// the result is scaled by 4*N*N, N is block width/height
// to visualize, perform normalize(value/(4*N*N)) for each block
void mat_idct_blks(xMat mat, int dim)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    xBlock blk = blk_calloc(dim, dim), idct_blk = blk_calloc(dim, dim);

    for (int i = 0; i < w * h / (dim * dim); i++) {
        mat_get_blk(mat, blk, i);
        idct(idct_blk, blk, dim, dim);
        mat_set_blk(mat, idct_blk, i);
    }

    blk_free(blk);
    blk_free(idct_blk);
}
