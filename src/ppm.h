#pragma once

#include <stdint.h>
#include <stdio.h>

/*
P6
# comment
3 2
255
0x00 0x00 0x00
*/

// P6 only
struct PPM {
    char magic[2];     // P3 for ASCII, P6 for binary
    int width, height; // header always be ASCII
    int colors;        // supported colors number per pixel(0-65535)
    int pitch;         // bytes per row (generally no padding)
    uint8_t *data;     // data section, each R/G/B may be 1byte(colors < 256) or
                       // 2bytes (colors > 256) packed? big/littel endian?
};

void ppm_dump_header(const struct PPM *ppm);
int ppm_read_header(FILE *f, struct PPM *ppm);
int ppm_read_data(FILE *f, struct PPM *ppm);
void ppm_free(struct PPM *ppm);
struct PPM *ppm_read_file(const char *name);
