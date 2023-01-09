#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum Channel {
    CHAN_Y = 0x01,
    CHAN_U = 0x02,
    CHAN_V = 0x04,
    CHAN_R = 0x08,
    CHAN_G = 0x10,
    CHAN_B = 0x20,
} Channel;

// each format value is bitwise `or`ed channels
typedef enum PixelFormat {
    FMT_YUV420 = CHAN_Y | CHAN_U | CHAN_V,
    FMT_RGB24 = CHAN_R | CHAN_G | CHAN_B,
} PixelFormat;

size_t fmt_get_size(PixelFormat fmt, size_t w, size_t h);
size_t fmt_get_pitch(PixelFormat fmt, size_t w, size_t h);

// a uint8_t 2d pixel buffer
typedef struct PixelBuffer {
    PixelFormat fmt;
    // the width/height in pixels
    size_t w, h;
    // the buf size in bytes
    size_t size;
    // a continuous array
    uint8_t buf[0];
} PixelBuffer;

PixelBuffer *pxb_new(PixelFormat fmt, size_t w, size_t h, const uint8_t *buf);
PixelBuffer *pxb_copy(const PixelBuffer *src, int mask);
void pxb_free(PixelBuffer *pxb);
void pxb_remove_channels(PixelBuffer *pxb, int mask);
