#ifndef _YUV_H_
#define _YUV_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t y, u, v;
} YUV;

typedef struct {
    uint8_t r, g, b;
} RGB;

YUV yuv_from_rgb(uint8_t r, uint8_t g, uint8_t b);

void rgb24_to_yuv444(size_t w, size_t h, uint8_t *src, uint8_t *dst);
void rgb24_to_yuv420(size_t w, size_t h, uint8_t *src, uint8_t *dst);

#ifdef __cplusplus
}
#endif
#endif
