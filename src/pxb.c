#include "pxb.h"

size_t fmt_get_size(PixelFormat fmt, size_t w, size_t h)
{

    switch (fmt) {
    case FMT_YUV420:
        return w * h * 3 / 2;
    case FMT_RGB24:
        return w * h * 3;
    default:
        return w * h * 3 / 2;
    }
}

size_t fmt_get_pitch(PixelFormat fmt, size_t w, size_t h)
{
    switch (fmt) {
    case FMT_YUV420:
        return w;
    case FMT_RGB24:
        return w * 3;
    default:
        return w;
    }
}

PixelBuffer *pxb_new(PixelFormat fmt, size_t w, size_t h, const uint8_t *buf)
{
    size_t size = fmt_get_size(fmt, w, h);
    PixelBuffer *pxb = malloc(sizeof(PixelBuffer) + size);
    pxb->fmt = fmt;
    pxb->w = w;
    pxb->h = h;
    pxb->size = size;

    if (buf) {
        memcpy(pxb->buf, buf, size);
    }
    return pxb;
}

void pxb_free(PixelBuffer *pxb) { free(pxb); }

PixelBuffer *pxb_copy(const PixelBuffer *src, int mask)
{
    PixelBuffer *pxb = malloc(sizeof(PixelBuffer) + src->size);
    memcpy(pxb, src, sizeof(PixelBuffer) + src->size);

    pxb_remove_channels(pxb, mask ^ pxb->fmt);
    return pxb;
}

void pxb_remove_channels(PixelBuffer *pxb, int mask)
{
    switch (pxb->fmt) {
    case FMT_YUV420:
        if (mask & CHAN_Y)
            memset(pxb->buf + pxb->size * 0 / 6, 128, pxb->size * 4 / 6);
        if (mask & CHAN_U)
            memset(pxb->buf + pxb->size * 4 / 6, 128, pxb->size * 1 / 6);
        if (mask & CHAN_V)
            memset(pxb->buf + pxb->size * 5 / 6, 128, pxb->size * 1 / 6);
        break;
    case FMT_RGB24:
        // TODO: simd? endian?
        for (int i = 0; i < pxb->size / 3; i++) {
            if (mask & CHAN_R)
                memset(pxb->buf + i * 3 + 0, 0, 1);
            if (mask & CHAN_G)
                memset(pxb->buf + i * 3 + 1, 0, 1);
            if (mask & CHAN_B)
                memset(pxb->buf + i * 3 + 2, 0, 1);
        }
        break;
    default:
        break;
    }
}
