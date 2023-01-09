#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blk.h"

static inline int min(int x, int y) { return x > y ? y : x; }
// static inline int max(int x, int y) { return x > y ? x : y; } // NOLINT

xBlock blk_calloc(size_t dimX, size_t dimY)
{
    xReal **m = calloc(dimX, sizeof(xReal *));
    xReal *data = calloc(dimX * dimY + 2, sizeof(xReal));
    xReal *head = data + 2;

    data[0] = (xReal)dimX;
    data[1] = (xReal)dimY;

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
    free((xReal **)blk);
}

size_t blk_get_width(xBlock blk) { return blk[0][-2]; }

size_t blk_get_height(xBlock blk) { return blk[0][-1]; }

void blk_foreachi(xBlock blk, xBlkIterFn iter_func, void *payload)
{
    if (iter_func == NULL)
        return;

    int w = blk_get_width(blk);
    int h = blk_get_height(blk);

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            blk[i][j] = iter_func(blk, blk[i][j], i, j, payload);
        }
    }
}

void blk_add(xBlock a, xBlock b, xBlock c)
{
    int w = blk_get_width(a);
    int h = blk_get_height(a);
    for (int i = 0; i < w * h; i++) {
        c[0][i] = a[0][i] + b[0][i];
    }
}

void blk_diff(xBlock a, xBlock b, xBlock c)
{

    int w = blk_get_width(a);
    int h = blk_get_height(a);
    for (int i = 0; i < w * h; i++) {
        c[0][i] = a[0][i] - b[0][i];
    }
}

void blk_product(xBlock a, xBlock b, xBlock c)
{

    int w = blk_get_width(a);
    int h = blk_get_height(a);
    for (int i = 0; i < w * h; i++) {
        c[0][i] = a[0][i] * b[0][i];
    }
}

void blk_devide(xBlock a, xBlock b, xBlock c)
{

    int w = blk_get_width(a);
    int h = blk_get_height(a);
    for (int i = 0; i < w * h; i++) {
        c[0][i] = a[0][i] / b[0][i]; // devide by zero?
    }
}

void blk_add_n(xBlock in, xReal n, xBlock out)
{
    int w = blk_get_width(in);
    int h = blk_get_height(in);
    for (int i = 0; i < w * h; i++) {
        out[0][i] = in[0][i] + n;
    }
}

void blk_product_n(xBlock in, xReal n, xBlock out)
{
    int w = blk_get_width(in);
    int h = blk_get_height(in);
    for (int i = 0; i < w * h; i++) {
        out[0][i] = in[0][i] * n;
    }
}

void blk_clear(xBlock blk, xReal n)
{
    int w = blk_get_width(blk);
    int h = blk_get_height(blk);

    // don't use memset for float point
    for (int i = 0; i < w * h; i++)
        blk[0][i] = n;
}

void blk_print(const char *name, xBlock blk, int idx)
{
    int w = blk_get_width(blk);
    int h = blk_get_height(blk);

    printf("%s blk [%d][%d] idx=%d \n", name, h, w, idx);
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            printf("%+3.8f\t", blk[i][j]);
        }
        printf("\n");
    }
}

void blk_zigzag(xBlock in, xBlock out)
{
    int m = blk_get_width(in);
    int n, i, j;

    for (i = n = 0; i < m * 2; i++)
        for (j = (i < m) ? 0 : i - m + 1; j <= i && j < m; j++)
            out[0][n++] = in[0][(i & 1) ? j * (m - 1) + i : (i - j) * m + j];
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

void mat_foreach_blk(xMat mat, int dim, xMatIterFn iter_func, void *payload)
{
    if (iter_func == NULL)
        return;

    size_t w = mat_get_width(mat), h = mat_get_height(mat);

    xBlock blk = blk_calloc(dim, dim);

    for (int i = 0; i < w * h / (dim * dim); i++) {
        mat_get_blk(mat, blk, i);
        iter_func(mat, blk, i, payload);
        mat_set_blk(mat, blk, i);
    }

    blk_free(blk);
}

void mat_get_blk(const xMat mat, xBlock blk, int idx)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    size_t dim = blk_get_height(blk);

    int rows = min(h - idx / (w / dim) * dim, dim);
    int cols = min(w - idx % (w / dim) * dim, dim);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            blk[i][j] = mat[(idx / (w / dim) * dim + i) * w +
                            (idx % (w / dim) * dim + j)];
        }
    }
}

void mat_set_blk(xMat mat, xBlock blk, int idx)
{
    size_t w = mat_get_width(mat), h = mat_get_height(mat);
    size_t dim = blk_get_height(blk);

    int rows = min(h - idx / (w / dim) * dim, dim);
    int cols = min(w - idx % (w / dim) * dim, dim);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            mat[(idx / (w / dim) * dim + i) * w + (idx % (w / dim) * dim + j)] =
                blk[i][j];
        }
    }
}

void mat_add(xMat a, xMat b, xMat c)
{
    int w = mat_get_width(a);
    int h = mat_get_height(a);

    for (int i = 0; i < w * h; i++) {
        c[i] = a[i] + b[i];
    }
}

void mat_diff(xMat a, xMat b, xMat c)
{
    int w = mat_get_width(a);
    int h = mat_get_height(a);

    for (int i = 0; i < w * h; i++) {
        c[i] = a[i] - b[i];
    }
};

void mat_product(xMat a, xMat b, xMat c)
{
    int w = mat_get_width(a);
    int h = mat_get_height(a);

    for (int i = 0; i < w * h; i++) {
        c[i] = a[i] * b[i];
    }
}

void mat_devide(xMat a, xMat b, xMat c)
{
    int w = mat_get_width(a);
    int h = mat_get_height(a);

    for (int i = 0; i < w * h; i++) {
        c[i] = a[i] / b[i]; // devide by zero?
    }
}

void mat_add_n(xMat in, xReal n, xMat out)
{

    int w = mat_get_width(in);
    int h = mat_get_height(in);

    for (int i = 0; i < w * h; i++) {
        out[i] = in[i] + n;
    }
}

void mat_product_n(xMat in, xReal n, xMat out)
{

    int w = mat_get_width(in);
    int h = mat_get_height(in);

    for (int i = 0; i < w * h; i++) {
        out[i] = in[i] * n;
    }
}
