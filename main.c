#include <stdarg.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_surface.h>

#include "src/dct.h"
#include "src/ppm.h"
#include "src/pxb.h"
#include "src/yuv.h"

#define N 8

typedef struct PreviewWindow {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} PreviewWindow;

static int interrupted = 0;

static inline uint32_t get_fmt(PixelFormat fmt)
{
    switch (fmt) {
    case FMT_YUV420:
        return SDL_PIXELFORMAT_IYUV;
    case FMT_RGB24:
        return SDL_PIXELFORMAT_RGB24;
    default:
        return SDL_PIXELFORMAT_IYUV;
    }
}

static int handle_window_event()
{
    SDL_Event ev;
    int ret = SDL_PollEvent(&ev);
    if (ret <= 0) {
        return ret;
    }

    switch (ev.type) {
    case SDL_WINDOWEVENT:
        if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
            SDL_Window *w = SDL_GetWindowFromID(ev.window.windowID);
            SDL_HideWindow(w);
        }
        break;
    case SDL_QUIT:
        interrupted = 1;
        return -1;
    case SDL_KEYUP:
        switch (ev.key.keysym.sym) {
        case SDLK_q:
            interrupted = 1;
            return -1;
        case SDLK_SPACE:
            break;
        case SDLK_RETURN:
            break;
        case SDLK_ESCAPE:
            break;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        break;
    case SDL_MOUSEMOTION:
        break;
    default:
        return 0;
    }
    return 0;
}

PreviewWindow *create_preview_window(const char *title, PixelBuffer *pxb)
{
    uint32_t format = get_fmt(pxb->fmt);
    size_t pitch = fmt_get_pitch(pxb->fmt, pxb->w, pxb->h);
    SDL_Rect rect = {.w = pxb->w, .h = pxb->h};

    PreviewWindow *win = malloc(sizeof(PreviewWindow));
    SDL_Window *window =
        SDL_CreateWindow(title, rect.x, rect.y, rect.w * 2, rect.h * 2, 0);
    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(
        renderer, format, SDL_TEXTUREACCESS_STATIC, rect.w, rect.h);

    SDL_UpdateTexture(texture, &rect, pxb->buf, pitch);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);
    SDL_RenderFlush(renderer);
    SDL_ShowWindow(window);

    win->window = window;
    win->renderer = renderer;
    win->texture = texture;

    return win;
}

void destroy_preview_window(PreviewWindow *win)
{

    SDL_DestroyTexture(win->texture);
    SDL_DestroyRenderer(win->renderer);
    SDL_DestroyWindow(win->window);
    free(win);
}

void draw_raster(uint8_t *pixels, size_t w, size_t h)
{
    // draw raster
    for (int i = 0; i < h; i += N) {
        for (int j = 0; j < w; j += N) {
            pixels[i * w + j] = 255;
        }
    }
}

void float_from_uint8_t(float *dst, uint8_t *src, size_t size)
{
    for (int i = 0; i < size; i++) {
        dst[i] = (float)src[i];
    }
}

void float_to_uint8_t(uint8_t *dst, float *src, size_t size)
{
    for (int i = 0; i < size; i++) {
        dst[i] = (uint8_t)(src[i]);
    }
}

static inline xReal min(xReal x, xReal y) { return x > y ? y : x; }

static xReal normalize(xReal x, int i, int j, void *payload)
{
    struct _Payload {
        xBlock blk;
        void *payload;
    };

    xReal *minmax = ((struct _Payload *)payload)->payload;
    xReal minv = minmax[0], maxv = minmax[1];

    return 255. * (x - minv) / (maxv - minv);
}

static xReal rescale(xReal x, int i, int j, void *_payload)
{
    return x / (4 * N * N);
}

static xBlock normalize_block(xBlock blk, int idx, void *_payload)
{
    xReal minmax[2] = {blk[0][0], blk[0][0]};

    for (int i = 0; i < N * N; i++) {
        minmax[0] = min(minmax[0], blk[0][i]);
        // no need to calculate maxv,
        // since it should always be blk[0][0]
    }
    blk_foreach(blk, normalize, minmax);
    return blk;
}

static xBlock rescale_block(xBlock blk, int idx, void *_payload)
{
    blk_foreach(blk, rescale, NULL);
    return blk;
}

/*
 * 1. rgb split to yuv plannar
 * 2. yuv subsampling to yuv420p
 * 3. split to 8x8 blocks for each plannar
 * 4. DCT <- current here
 * 5. run-length compression
 * 6. entropy
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s <ppm_file>\n", argv[0]);
        return -1;
    }

    int ret;
    ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0) {
        fprintf(stderr, "failed to init sdl2: %s\n", SDL_GetError());
        exit(-1);
    }

    struct PPM *ppm = ppm_read_file(argv[1]);
    size_t w = ppm->width, h = ppm->height;

    PixelBuffer *rgb_buf = pxb_new(FMT_RGB24, w, h, ppm->data);
    PixelBuffer *yuv_buf = pxb_new(FMT_YUV420, w, h, NULL);

    ppm_free(ppm);

    rgb24_to_yuv420(w, h, rgb_buf->buf, yuv_buf->buf);

    // show split y/u/v plane
    PixelBuffer *yplane = pxb_copy(yuv_buf, CHAN_Y);

    xMat mat = mat_calloc(yplane->size), idct_mat;
    xBlock blk = blk_calloc(N, N);

    // DCT
    float_from_uint8_t(mat, yplane->buf, yplane->size);
    mat_get_blk(mat, blk, 0, w, h);
    blk_print("raw", blk, N, 0);

    PixelBuffer *dctplane = pxb_copy(yplane, CHAN_Y);
    mat_dct_blks(mat, w, h);
    idct_mat = mat_copy(mat, yplane->size);
    mat_get_blk(mat, blk, 0, w, h);
    blk_print("dct", blk, N, 0);
    mat_foreach_blk(mat, w, h, normalize_block, NULL);
    mat_get_blk(mat, blk, 0, w, h);
    blk_print("normalized dct", blk, N, 0);
    float_to_uint8_t(dctplane->buf, mat, dctplane->size);

    // inverse DCT
    PixelBuffer *idctplane = pxb_copy(yplane, CHAN_Y);
    mat_idct_blks(idct_mat, w, h);
    mat_get_blk(idct_mat, blk, 0, w, h);
    blk_print("idct", blk, N, 0);
    mat_foreach_blk(idct_mat, w, h, rescale_block, NULL);
    mat_get_blk(idct_mat, blk, 0, w, h);
    blk_print("rescaled idct", blk, N, 0);
    float_to_uint8_t(idctplane->buf, idct_mat, idctplane->size);

    mat_free(mat);

    draw_raster(yplane->buf, w, h);

    // show each window
    PreviewWindow *yuvwin = create_preview_window("raw yuv", yuv_buf);
    PreviewWindow *ywin = create_preview_window("y plane", yplane);
    PreviewWindow *dct_win = create_preview_window("dct y plane", dctplane);
    PreviewWindow *idct_win = create_preview_window("idct y plane", idctplane);

    while (interrupted == 0) {
        handle_window_event();
    }

    // EXIT:
    pxb_free(idctplane);
    pxb_free(dctplane);
    pxb_free(yplane);
    pxb_free(yuv_buf);
    pxb_free(rgb_buf);

    destroy_preview_window(idct_win);
    destroy_preview_window(dct_win);
    destroy_preview_window(ywin);
    destroy_preview_window(yuvwin);

    printf("bye\n");
}
