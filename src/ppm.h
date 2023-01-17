#ifndef _PPM_H_
#define _PPM_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdio.h>

/*
P6
# comment
3 2
255
0x00 0x00 0x00
*/

// P1-P3 not test yet
typedef enum PPM_FMT {
    PPM_FMT_P1 = 1, // bitmap ASCII
    PPM_FMT_P2,     // graymap ASCII
    PPM_FMT_P3,     // pixmap ASCII
    PPM_FMT_P4,     // bitmap binary
    PPM_FMT_P5,     // graymap binary
    PPM_FMT_P6,     // pixmap binary
} PPM_FMT;

int ppm_fmt_get_pix_colors(PPM_FMT fmt);
int ppm_fmt_get_pix_bits(PPM_FMT fmt);

// TODO: remove pitch? add pad field for conveniently write to header
typedef struct PPM {
    char magic[2];     // P3 for ASCII, P6 for binary
    int width, height; // header always be ASCII
    int colors;        // supported colors number per pixel(0-65535)
    int pitch;         // bytes per row (generally no padding)
    uint8_t *data;     // data section, each R/G/B may be 1byte(colors < 256) or
                       // 2bytes (colors > 256) packed? big/littel endian?
} PPM;

// TODO: define and handle errors
PPM *ppm_create(PPM_FMT fmt, size_t w, size_t h, uint8_t *data);
void ppm_free(PPM *ppm);
void ppm_dump_header(const PPM *ppm);
int ppm_read_header(FILE *f, PPM *ppm);
int ppm_read_data(FILE *f, PPM *ppm);
PPM *ppm_read_file(const char *name);
int ppm_write_file(PPM *ppm, const char *name);

#ifdef __cplusplus
}
#endif
#endif
