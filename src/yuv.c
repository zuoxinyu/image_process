#include <math.h>

#include "yuv.h"

#define CLIP(X) ((X) > 255 ? 255 : (X) < 0 ? 0 : X)

// RGB -> YUV
#define RGB2Y(R, G, B) CLIP(((66 * (R) + 129 * (G) + 25 * (B) + 128) >> 8) + 16)
#define RGB2U(R, G, B) CLIP(((-38 * (R)-74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLIP(((112 * (R)-94 * (G)-18 * (B) + 128) >> 8) + 128)

// YUV -> RGB
#define C(Y) ((Y)-16)
#define D(U) ((U)-128)
#define E(V) ((V)-128)

#define YUV2R(Y, U, V) CLIP((298 * C(Y) + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP((298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U, V) CLIP((298 * C(Y) + 516 * D(U) + 128) >> 8)

// RGB -> YCbCr
#define CRGB2Y(R, G, B) CLIP((19595 * R + 38470 * G + 7471 * B) >> 16)
#define CRGB2Cb(R, G, B)                                                       \
    CLIP(                                                                      \
        (36962 * (B - CLIP((19595 * R + 38470 * G + 7471 * B) >> 16)) >> 16) + \
        128)
#define CRGB2Cr(R, G, B)                                                       \
    CLIP(                                                                      \
        (46727 * (R - CLIP((19595 * R + 38470 * G + 7471 * B) >> 16)) >> 16) + \
        128)

// YCbCr -> RGB
#define CYCbCr2R(Y, Cb, Cr) CLIP(Y + (91881 * Cr >> 16) - 179)
#define CYCbCr2G(Y, Cb, Cr) CLIP(Y - ((22544 * Cb + 46793 * Cr) >> 16) + 135)
#define CYCbCr2B(Y, Cb, Cr) CLIP(Y + (116129 * Cb >> 16) - 226)

double clamp(double x, double lower, double upper)
{
    x = x > upper ? upper : x;
    x = x < lower ? lower : x;
    return x;
}

YUV yuv_from_rgb(uint8_t r, uint8_t g, uint8_t b)
{

    YUV yuv = {0};
    /*
    yuv.y = clamp(0.299 * r + 0.587 * g + 0.114 * b, 0, 255);
    yuv.u = clamp(-0.147 * r - 0.289 * g + 0.436 * b, -128, 127);
    yuv.v = clamp(0.615 * r - 0.515 * g - 0.100 * b, -128, 127);

    yuv.y = RGB2Y(r, g, b);
    yuv.u = RGB2U(r, g, b);
    yuv.v = RGB2V(r, g, b);
    */

    yuv.y = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
    yuv.v = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
    yuv.u = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;

    return yuv;
}

void rgb24_to_yuv444(size_t w, size_t h, uint8_t *src, uint8_t *dst)
{
    uint8_t *y = dst;
    uint8_t *u = y + w * h;
    uint8_t *v = u + w * h;
    YUV yuv;

    for (int i = 0; i < w * h; ++i) {
        yuv = yuv_from_rgb(src[i * 3], src[i * 3 + 1], src[i * 3 + 2]);
        y[i] = yuv.y;
        u[i] = yuv.u;
        v[i] = yuv.v;
    }
}

void rgb24_to_yuv420(size_t w, size_t h, uint8_t *src, uint8_t *dst)
{
    uint8_t *y = dst;
    uint8_t *u = y + w * h;
    uint8_t *v = u + w * h / 4;
    uint8_t *up = u, *vp = v;
    YUV yuv;

    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            int pos = i * w + j;

            yuv =
                yuv_from_rgb(src[pos * 3], src[pos * 3 + 1], src[pos * 3 + 2]);

            y[pos] = yuv.y;

            if (i % 2 == 0 && j % 2 == 0) {
                *up++ = yuv.u;
                *vp++ = yuv.v;
            }
        }
    }
}
