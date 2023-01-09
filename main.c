#include <limits.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>

#include "src/blk.h"
#include "src/dct.h"
#include "src/ppm.h"
#include "src/pxb.h"
#include "src/rle.h"
#include "src/yuv.h"

#include "window.h"

#define N 8
#define BLKID 0

#define ANSI_COLOR_YELLOW "\033[0;32m"
#define ANSI_COLOR_RESET "\033[0m"

#define blk_print(title, blk, idx)                                             \
    do {                                                                       \
        if (N > 16) {                                                          \
            break;                                                             \
        }                                                                      \
        const char *color_title = ANSI_COLOR_YELLOW title ANSI_COLOR_RESET;    \
        blk_print(color_title, (blk), (idx));                                  \
    } while (0)

// clang-format off
static const xReal QUANTIZE_TBLS[][8][8] = {
    {
        { 112.,  84.,  98.,  98., 126., 168., 343., 504. },
        {  77.,  84.,  91., 119., 154., 245., 448., 644. },
        {  70.,  98., 112., 154., 259., 385., 546., 665. },
        { 112., 133., 168., 203., 392., 448., 609., 686. },
        { 168., 182., 280., 357., 476., 567., 721., 784. },
        { 280., 406., 399., 609., 763., 728., 847., 700. },
        { 357., 420., 483., 560., 721., 791., 840., 721. },
        { 427., 385., 392., 434., 539., 644., 707., 693. },
    },
    {
        {  74.,  55.,  64.,  64.,  83., 110., 225., 331. },
        {  51.,  55.,  60.,  78., 101., 161., 294., 423. },
        {  46.,  64.,  74., 101., 170., 253., 359., 437. },
        {  74.,  87., 110., 133., 258., 294., 400., 451. },
        { 110., 120., 184., 235., 313., 373., 474., 515. },
        { 184., 267., 262., 400., 501., 478., 557., 460. },
        { 235., 276., 317., 368., 474., 520., 552., 474. },
        { 281., 253., 258., 285., 354., 423., 465., 455. },
    },
    {
        { 16.,  12.,  14.,  14.,  18.,  24.,  49.,  72. },
        { 11.,  12.,  13.,  17.,  22.,  35.,  64.,  92. },
        { 10.,  14.,  16.,  22.,  37.,  55.,  78.,  95. },
        { 16.,  19.,  24.,  29.,  56.,  64.,  87.,  98. },
        { 24.,  26.,  40.,  51.,  68.,  81., 103., 112. },
        { 40.,  58.,  57.,  87., 109., 104., 121., 100. },
        { 51.,  60.,  69.,  80., 103., 113., 120., 103. },
        { 61.,  55.,  56.,  62.,  77.,  92., 101.,  99. },
    },
    {
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
        { 1., 1., 1., 1., 1., 1., 1., 1. },
    },
};
// clang-format on

static int interrupted = 0;
static int QF = 1;

static int handle_mouse_click(SDL_Window *w, SDL_Event ev)
{
    int x = ev.button.x;
    int y = ev.button.y;

    // TODO: draw block details
    printf("click at: %d,%d\n", x, y);

    return 0;
}

static int handle_scale(PreviewWindow *pw, int step)
{

    int w, h;
    char title[128] = {0};

    SDL_GetWindowSize(pw->window, &w, &h);
    double factor = ((double)w / pw->pxb->w) + step * WIN_SCALE_FACTOR;
    snprintf(title, 128, "%s - [*%0.2lf]", pw->title, factor);

    SDL_SetWindowTitle(pw->window, title);
    SDL_SetWindowSize(pw->window, pw->pxb->w * factor, pw->pxb->h * factor);
    SDL_RenderCopy(pw->renderer, pw->texture, NULL, NULL);
    SDL_RenderPresent(pw->renderer);
    SDL_RenderFlush(pw->renderer);

    return 0;
}

static int handle_window_event()
{
    SDL_Event ev;
    int ret = SDL_PollEvent(&ev);
    if (ret <= 0) {
        return ret;
    }
    SDL_Window *win = SDL_GetWindowFromID(ev.window.windowID);
    PreviewWindow *pw = SDL_GetWindowData(win, OPAQUE_NAME);

    switch (ev.type) {
    case SDL_WINDOWEVENT:
        if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
            SDL_HideWindow(win);
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
        case SDLK_ESCAPE:
            SDL_HideWindow(win);
            break;
        case SDLK_u:
            handle_scale(pw, 1);
            break;
        case SDLK_d:
            handle_scale(pw, -1);
            break;
        case SDLK_1:
            break;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        handle_mouse_click(win, ev);
        break;
    case SDL_MOUSEMOTION:
        break;
    default:
        return 0;
    }
    return 0;
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
static inline xReal max(xReal x, xReal y) { return x < y ? y : x; }

static xReal lshift128(xBlock blk, xReal x, int i, int j, void *payload)
{
    return x - 128;
}

static xReal rshift128(xBlock blk, xReal x, int i, int j, void *payload)
{
    return x + 128;
}

static xReal normalize(xBlock blk, xReal x, int i, int j, void *payload)
{

    xReal *minmax = payload;
    xReal minv = minmax[0], maxv = minmax[1];

    return 255. * (x - minv) / (maxv - minv);
}

static xReal quantize(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return roundf(x / QUANTIZE_TBLS[QF][i][j]);
}

static xReal reverse(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return 128. - (x > 0 ? x : -x);
}

static xReal dequantize(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return roundf(x * QUANTIZE_TBLS[QF][i][j]);
}

static xBlock lshift128_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreachi(blk, lshift128, NULL);
    return blk;
}

static xBlock rshift128_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreachi(blk, rshift128, NULL);
    return blk;
}

static xBlock reverse_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreachi(blk, reverse, NULL);
    return blk;
}

static xBlock normalize_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    xReal minmax[2] = {blk[0][0], blk[0][0]};

    for (int i = 0; i < N * N; i++) {
        minmax[0] = min(minmax[0], blk[0][i]);
        minmax[1] = max(minmax[1], blk[0][i]);
    }
    blk_foreachi(blk, normalize, minmax);
    return blk;
}

static xBlock quantize_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreachi(blk, quantize, NULL);
    return blk;
}

static xBlock dequantize_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreachi(blk, dequantize, NULL);
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

    const char *file_name = argv[1];

    int ret;
    ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0) {
        fprintf(stderr, "failed to init sdl2: %s\n", SDL_GetError());
        exit(-1);
    }

    PPM *ppm = ppm_read_file(file_name);
    size_t w = ppm->width, h = ppm->height;

    PixelBuffer *rgb_buf = pxb_new(FMT_RGB24, w, h, ppm->data);
    PixelBuffer *yuv_buf = pxb_new(FMT_YUV420, w, h, NULL);

    ppm_free(ppm);

    printf("\n========encoding========\n");
    // rgb to yuv and subsampling
    rgb24_to_yuv420(w, h, rgb_buf->buf, yuv_buf->buf);

    // extract y plane
    PixelBuffer *yplane = pxb_copy(yuv_buf, CHAN_Y);
    PixelBuffer *dctplane = pxb_copy(yplane, CHAN_Y);
    PixelBuffer *idctplane = pxb_copy(yplane, CHAN_Y);
    PixelBuffer *diffplane = pxb_copy(yplane, CHAN_Y);

    xMat mat = mat_calloc(w, h), orig_mat, dct_mat, idct_mat, diff_mat;
    xBlock blk = blk_calloc(N, N);

    float_from_uint8_t(mat, yplane->buf, w * h);
    orig_mat = mat_copy(mat);
    diff_mat = mat_copy(mat);

    // raw blk
    mat_get_blk(mat, blk, BLKID);
    blk_print("raw", blk, BLKID);

    // left shift 128
    mat_foreach_blk(mat, N, lshift128_block, NULL);
    mat_get_blk(mat, blk, BLKID);
    blk_print("lshift raw", blk, BLKID);

    // DCT
    mat_dct_blks(mat, N);
#ifdef USE_FFTW3
    mat_product_n(mat, 2. / (N * N), mat);
#endif
    mat_get_blk(mat, blk, BLKID);
    blk_print("dct", blk, BLKID);

    idct_mat = mat_copy(mat);
    dct_mat = mat_copy(mat);

    // quantize
    mat_foreach_blk(mat, N, quantize_block, NULL);
    mat_get_blk(mat, blk, BLKID);
    blk_print("quantized", blk, BLKID);

    // zigzag
    xBlock zigzag_blk = blk_copy(blk);
    blk_zigzag(blk, zigzag_blk);
    blk_print("zigzag", zigzag_blk, BLKID);

    // TODO: diff DC encoding

    // run-length
    xRLETable tbl = rtb_calloc(N * N);
    int tbl_len = run_length(tbl, zigzag_blk);
    rtb_print(tbl, tbl_len);

    // huffman
    for (int i = 0; i < tbl_len; i++) {
        
    }

    printf("\n========decoding========\n");

    // normalize DCT for showing
    mat_foreach_blk(dct_mat, N, normalize_block, NULL);
    mat_get_blk(dct_mat, blk, BLKID);
    blk_print("normalized dct", blk, BLKID);

    // inverse DCT
    mat_foreach_blk(idct_mat, N, quantize_block, NULL);
    mat_foreach_blk(idct_mat, N, dequantize_block, NULL);

#ifdef USE_FFTW3
    mat_product_n(idct_mat, N * N / 2., idct_mat);
#endif
    mat_idct_blks(idct_mat, N);
#ifdef USE_FFTW3
    mat_product_n(idct_mat, 1. / (4. * N * N), idct_mat);
#endif
    mat_get_blk(idct_mat, blk, BLKID);
    blk_print("idct", blk, BLKID);

    // right shift 128
    mat_foreach_blk(idct_mat, N, rshift128_block, NULL);
    mat_get_blk(idct_mat, blk, BLKID);
    blk_print("rshift idct", blk, BLKID);

    mat_diff(orig_mat, idct_mat, diff_mat);
    mat_get_blk(diff_mat, blk, BLKID);
    mat_foreach_blk(diff_mat, N, reverse_block, NULL);
    blk_print("diff idct", blk, BLKID);

    // copy back to uint8 buffer for showing
    float_to_uint8_t(dctplane->buf, dct_mat, w * h);
    float_to_uint8_t(idctplane->buf, idct_mat, w * h);
    float_to_uint8_t(diffplane->buf, diff_mat, w * h);

    /* draw_raster(yplane->buf, w, h); */
    /* draw_raster(idctplane->buf, w, h); */

    // show each window
    PreviewWindow *yuv_win = create_preview_window("raw yuv", yuv_buf);
    PreviewWindow *y_win = create_preview_window("y plane", yplane);
    PreviewWindow *dct_win = create_preview_window("dct y plane", dctplane);
    PreviewWindow *idct_win = create_preview_window("idct y plane", idctplane);
    PreviewWindow *diff_win = create_preview_window("diff y plane", diffplane);

    while (interrupted == 0) {
        handle_window_event();
    }

    // EXIT:
    rtb_free(tbl);
    mat_free(mat);
    mat_free(dct_mat);
    mat_free(idct_mat);
    blk_free(zigzag_blk);
    blk_free(blk);

    pxb_free(diffplane);
    pxb_free(idctplane);
    pxb_free(dctplane);
    pxb_free(yplane);
    pxb_free(yuv_buf);
    pxb_free(rgb_buf);

    destroy_preview_window(diff_win);
    destroy_preview_window(idct_win);
    destroy_preview_window(dct_win);
    destroy_preview_window(y_win);
    destroy_preview_window(yuv_win);

    printf("bye\n");
}
