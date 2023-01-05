#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ppm.h"

static int ppm_fmt_pix_bits_tbl[] = {
    [1] = 8, [2] = 8 * 3, [3] = 8 * 3 * 3, [4] = 1, [5] = 8, [6] = 8 * 3,
};

static int ppm_fmt_pix_colors_tbl[] = {
    [1] = 2, [2] = 255, [3] = 255, [4] = 2, [5] = 255, [6] = 255,
};

int ppm_fmt_get_pix_colors(PPM_FMT fmt) { return ppm_fmt_pix_colors_tbl[fmt]; }

int ppm_fmt_get_pix_bits(PPM_FMT fmt) { return ppm_fmt_pix_bits_tbl[fmt]; }

PPM *ppm_create(PPM_FMT fmt, size_t w, size_t h, uint8_t *data)
{
    PPM *ppm = malloc(sizeof(PPM));
    ppm->magic[0] = 'P';
    ppm->magic[1] = '0' + fmt;
    ppm->width = w;
    ppm->height = h;
    ppm->colors = ppm_fmt_get_pix_colors(fmt);
    ppm->pitch = w * ppm_fmt_get_pix_bits(fmt) / 8;
    ppm->data = NULL;
    if (data) {
        ppm->data = malloc(sizeof(uint8_t) * ppm->pitch * h);
        memcpy(ppm->data, data, sizeof(uint8_t) * ppm->pitch * h);
    }
    return ppm;
}

void ppm_free(PPM *ppm)
{
    if (ppm) {
        if (ppm->data)
            free(ppm->data);
        free(ppm);
    }
}

// dump the ppm header info
void ppm_dump_header(const PPM *ppm)
{
    printf("magic: %c%c\n", ppm->magic[0], ppm->magic[1]);
    printf("size:  %d %d\n", ppm->width, ppm->height);
    printf("color: %d\n", ppm->colors);
    printf("data:  [%d]\n", ppm->data ? ppm->height * ppm->pitch : -1);
}

// read header from file, data is not touched
// return negative value if error occurs
int ppm_read_header(FILE *f, PPM *ppm)
{
    char ch;
    int nread;

    nread = fread(ppm->magic, 1, 2, f);
    assert(nread == 2);

    if (ppm->magic[0] != 'P' || (ppm->magic[1] < '1' && ppm->magic[1] > '6')) {
        return -1;
    }

    nread = fread(&ch, 1, 1, f);
    assert(nread == 1);
    if (ch != '\n') {
        return -1;
    }

    int n = fscanf(f, "%d%d", &ppm->width, &ppm->height);
    assert(n == 2);

    if (ppm->magic[1] != '1' && ppm->magic[1] != '4') {
        n = fscanf(f, "%d", &ppm->colors);
        assert(n == 1);
    }

    if (ppm->colors <= 0 || ppm->colors > 65535) {
        return -1;
    }

    nread = fread(&ch, 1, 1, f);
    assert(ch == '\n' && nread == 1);

    ppm->pitch = (ppm->colors > 255 ? 2 : 1) * 3 * ppm->width;

    return 0;
}

// read data from file, acording to the header, data is mallocated
// must be called after ppm_read_header cause of file cursor
int ppm_read_data(FILE *f, PPM *ppm)
{
    int nsize = ppm->height * ppm->pitch;

    ppm->data = malloc(nsize);
    if (ppm->data == NULL) {
        perror("malloc\n");
        return -1;
    }

    int nread = fread(ppm->data, 1, nsize, f);
    assert(nread == nsize);

    return nread;
}

PPM *ppm_read_file(const char *name)
{
    FILE *fp = NULL;
    PPM *ppm = NULL;
    int ret = 0;

    fp = fopen(name, "rb");
    if (!fp)
        goto FAIL;

    ppm = calloc(sizeof(PPM), 1);
    if (!ppm)
        goto FAIL;

    ret = ppm_read_header(fp, ppm);
    if (ret < 0)
        goto FAIL;
    ret = ppm_read_data(fp, ppm);
    if (ret < 0)
        goto FAIL;
    ppm_dump_header(ppm);

    return ppm;

FAIL:
    if (ppm)
        ppm_free(ppm);
    if (fp)
        fclose(fp);
    return NULL;
}

int ppm_write_file(PPM *ppm, const char *name)
{
    FILE *fp = fopen(name, "wb");
    if (!fp)
        return -1;

    int ret;
    ret = fprintf(fp, "%c%c\n%d %d\n", ppm->magic[0], ppm->magic[1], ppm->width,
                  ppm->height);
    if (ppm->magic[1] != '1' && ppm->magic[1] != '4') {
        fprintf(fp, "%d\n", ppm->colors);
    }
    ret = fwrite(ppm->data, sizeof(uint8_t), ppm->pitch * ppm->height, fp);
    assert(ret == ppm->pitch * ppm->height);

    fflush(fp);
    fclose(fp);
    return 0;
}
