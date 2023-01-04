#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ppm.h"

// dump the ppm header info
void ppm_dump_header(const struct PPM *ppm)
{
    printf("magic: %c%c\n", ppm->magic[0], ppm->magic[1]);
    printf("size:  %d %d\n", ppm->width, ppm->height);
    printf("color: %d\n", ppm->colors);
    printf("pitch: %d\n", ppm->pitch);
    printf("data:  [%d]\n", ppm->height * ppm->pitch);
}

// read header from file, data is not touched
// return negative value if error occurs
int ppm_read_header(FILE *f, struct PPM *ppm)
{
    size_t nread = fread(ppm->magic, 1, 2, f);
    assert(nread == 2);
    if (ppm->magic[0] != 'P' || ppm->magic[1] != '6') {
        printf("not a PPM file\n");
        return -1;
    }

    int n = fscanf(f, "%d%d%d", &ppm->width, &ppm->height, &ppm->colors);
    assert(n == 3);

    char ch;
    nread = fread(&ch, 1, 1, f);
    assert(ch == '\n' && nread == 1);

    if (ppm->colors > 65535 || ppm->colors < 0) {
        printf("invalid color number\n");
        return -1;
    }
    ppm->pitch = (ppm->colors > 255 ? 2 : 1) * 3 * ppm->width;

    return 0;
}

// read data from file, acording to the header, data is mallocated
// must be called after ppm_read_header cause of file cursor
int ppm_read_data(FILE *f, struct PPM *ppm)
{
    if (ppm->magic[1] != '6') {
        printf("only P6 is supported\n");
        return -1;
    }

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

void ppm_free(struct PPM *ppm)
{
    if (ppm && ppm->data)
        free(ppm->data);
    if (ppm)
        free(ppm);
}

struct PPM *ppm_read_file(const char *name)
{

    FILE *fp = NULL;
    struct PPM *ppm = NULL;
    int ret = 0;

    fp = fopen(name, "rb");
    if (!fp)
        goto FAIL;

    ppm = calloc(sizeof(struct PPM), 1);
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

#ifdef TEST_PPM
int main(int argc, char *argv[])
{
    if (argc < 1) {
        printf("usage: %s <filename>\n", argv[0]);
        return -1;
    }

    struct PPM ppm = {0};
    FILE *f;
    int ret;

    f = fopen(argv[1], "rb");
    if (f == NULL) {
        return -1;
    }

    ret = ppm_read_header(f, &ppm);
    if (ret < 0)
        goto FAIL;
    ppm_dump_header(&ppm);

    ret = ppm_read_data(f, &ppm);
    if (ret < 0)
        goto FAIL;
    printf("data size: %d\n", ret);

FAIL:
    ppm_free_data(&ppm);
    fclose(f);
}
#endif
