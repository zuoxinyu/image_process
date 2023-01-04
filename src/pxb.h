#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum Channel {
    CHAN_Y = 0x01,
    CHAN_U = 0x02,
    CHAN_V = 0x04,
    CHAN_R = 0x08,
    CHAN_G = 0x10,
    CHAN_B = 0x20,
} Channel;

typedef enum PixelFormat {
    FMT_YUV420 = CHAN_Y | CHAN_U | CHAN_V,
    FMT_RGB24 = CHAN_R | CHAN_G | CHAN_B,
} PixelFormat;

size_t fmt_get_size(PixelFormat fmt, size_t w, size_t h);
size_t fmt_get_pitch(PixelFormat fmt, size_t w, size_t h);

typedef struct PixelBuffer {
    PixelFormat fmt;
    // size is the buf size in bytes, w/h is the width/height in pixels
    size_t w, h, size;
    // a simulated T[w][h] array, each `buf2d[i]` refs to `&buf[i*w]`
    uint8_t **buf2d;
    // a continuous array
    uint8_t buf[0];
} PixelBuffer;

PixelBuffer *pxb_new(PixelFormat fmt, size_t w, size_t h, const uint8_t *buf);
PixelBuffer *pxb_copy(const PixelBuffer *src, int mask);
void pxb_free(PixelBuffer *pxb);
void pxb_remove_channels(PixelBuffer *pxb, int mask);