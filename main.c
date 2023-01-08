#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <limits.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_surface.h>

#include "src/blk.h"
#include "src/dct.h"
#include "src/ppm.h"
#include "src/pxb.h"
#include "src/yuv.h"

#define N 8
#define BLKID 0
#define QF 1
#define WIN_SIZE_FACTOR 1

#define OPAQUE_NAME "previewwindow"
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

typedef struct PreviewWindow {
    PixelBuffer *pxb;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} PreviewWindow;

// clang-format off
static const xReal QUANTIZE_TBLS[][8][8] = {
    {
        { 112.,  84.,  98.,  98., 126., 168., 343., 504., },
        {  77.,  84.,  91., 119., 154., 245., 448., 644., },
        {  70.,  98., 112., 154., 259., 385., 546., 665., },
        { 112., 133., 168., 203., 392., 448., 609., 686., },
        { 168., 182., 280., 357., 476., 567., 721., 784., },
        { 280., 406., 399., 609., 763., 728., 847., 700., },
        { 357., 420., 483., 560., 721., 791., 840., 721., },
        { 427., 385., 392., 434., 539., 644., 707., 693., },
    },
    {
        {  93.,  70.,  81.,  81., 104., 139., 284., 418., },
        {  64.,  70.,  75.,  99., 128., 203., 371., 534., },
        {  58.,  81.,  93., 128., 215., 319., 452., 551., },
        {  93., 110., 139., 168., 325., 371., 505., 568., },
        { 139., 151., 232., 296., 394., 470., 597., 650., },
        { 232., 336., 331., 505., 632., 603., 702., 580., },
        { 296., 348., 400., 464., 597., 655., 696., 597., },
        { 354., 319., 325., 360., 447., 534., 586., 574., },
    },
    {
        { 13., 10., 12., 12., 15., 20., 40., 58. },
        {  9., 10., 11., 14., 18., 29., 52., 74. },
        {  9., 12., 13., 18., 30., 45., 63., 77. },
        { 13., 16., 20., 24., 45., 52., 70., 79. },
        { 20., 21., 33., 41., 55., 65., 83., 90. },
        { 33., 47., 46., 70., 88., 84., 97., 81. },
        { 41., 49., 56., 65., 83., 91., 97., 83. },
        { 49., 45., 45., 50., 62., 74., 81., 80. },
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

static int handle_mouse_click(SDL_Window *w, SDL_Event ev)
{
    int x = ev.button.x;
    int y = ev.button.y;

    // TODO: draw block details
    printf("click at: %d,%d\n", x, y);

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
    int w, h;
    SDL_GetWindowSize(win, &w, &h);

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
        case SDLK_SPACE:
            break;
        case SDLK_RETURN:
            break;
        case SDLK_ESCAPE:
            break;
        case SDLK_u:
            SDL_SetWindowSize(win, w * 1.1, h * 1.1);
            SDL_RenderCopy(pw->renderer, pw->texture, NULL, NULL);
            SDL_RenderPresent(pw->renderer);
            break;
        case SDLK_d:
            SDL_SetWindowSize(win, w / 1.1, h / 1.1);
            SDL_RenderCopy(pw->renderer, pw->texture, NULL, NULL);
            SDL_RenderPresent(pw->renderer);
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

PreviewWindow *create_preview_window(const char *title, PixelBuffer *pxb)
{
    uint32_t format = get_fmt(pxb->fmt);
    size_t pitch = fmt_get_pitch(pxb->fmt, pxb->w, pxb->h);
    SDL_Rect rect = {.w = pxb->w, .h = pxb->h};

    PreviewWindow *win = malloc(sizeof(PreviewWindow));
    SDL_Window *window =
        SDL_CreateWindow(title, rect.x, rect.y, rect.w * WIN_SIZE_FACTOR,
                         rect.h * WIN_SIZE_FACTOR, 0);
    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(
        renderer, format, SDL_TEXTUREACCESS_STATIC, rect.w, rect.h);

    SDL_UpdateTexture(texture, &rect, pxb->buf, pitch);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);
    SDL_RenderFlush(renderer);
    SDL_ShowWindow(window);

    SDL_SetWindowData(window, OPAQUE_NAME, win);

    win->pxb = pxb;
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

static xReal rescale(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return x / (4. * N * N);
}

static xReal quantize(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return roundf(x / QUANTIZE_TBLS[QF][i][j]);
}

static xReal dequantize(xBlock blk, xReal x, int i, int j, void *_payload)
{
    return roundf(x * QUANTIZE_TBLS[QF][i][j]);
}

static xBlock lshift128_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreach(blk, lshift128, NULL);
    return blk;
}

static xBlock rshift128_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreach(blk, rshift128, NULL);
    return blk;
}

static xBlock normalize_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    xReal minmax[2] = {blk[0][0], blk[0][0]};

    for (int i = 0; i < N * N; i++) {
        minmax[0] = min(minmax[0], blk[0][i]);
        minmax[1] = max(minmax[1], blk[0][i]);
    }
    blk_foreach(blk, normalize, minmax);
    return blk;
}

static xBlock quantize_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreach(blk, quantize, NULL);
    return blk;
}

static xBlock dequantize_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreach(blk, dequantize, NULL);
    return blk;
}

static xBlock rescale_block(xMat mat, xBlock blk, int idx, void *_payload)
{
    blk_foreach(blk, rescale, NULL);
    return blk;
}

static size_t calc_msb(int16_t n)
{
    if (n < 0)
        n *= -1;
#if __GNUC__
    return n == 0 ? 0 : 16 - __builtin_clz(n);
#else
    int nbits = 0;
    while (n >>= 1)
        nbits++;
    return nbits;
#endif
}

static int run_length(xRLETable tbl, xBlock blk)
{
    int w = blk_get_width(blk);
    int zeros = 0, j = 0;
    int16_t num, last = w * w - 1;

    while (blk[0][last] == 0) {
        last--;
    }

    for (int i = 1; i <= last; i++) {
        num = (int16_t)blk[0][i];
        if (zeros >= 16) {
            // 16 zeros
            tbl[j].zeros = 15;
            tbl[j].nbits = 0;
            tbl[j].amp = 0;
            zeros = 0;
            j++;
        } else if (num == 0) {
            zeros++;
        } else {
            tbl[j].zeros = zeros;
            tbl[j].nbits = calc_msb(num);
            tbl[j].amp = num;
            zeros = 0;
            j++;
        }
    }

    // EOB
    tbl[j].zeros = 0;
    tbl[j].nbits = 0;
    tbl[j].amp = 0;

    return j + 1;
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

    PPM *ppm = ppm_read_file(argv[1]);
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
    mat_get_blk(mat, blk, BLKID);
    blk_print("dct(unnormalized)", blk, BLKID);

    idct_mat = mat_copy(mat);
    dct_mat = mat_copy(mat);

    // rescale DCT for quantize
    // mat_foreach_blk(mat, N, rescale_block, NULL);
    // mat_get_blk(mat, blk, BLKID);
    // blk_print("rescaled dct", blk, BLKID);

    // quantize
    mat_foreach_blk(mat, N, quantize_block, NULL);
    mat_get_blk(mat, blk, BLKID);
    blk_print("50% quantized", blk, BLKID);

    // zigzag
    xBlock zigzag_blk = blk_copy(blk);
    blk_zigzag(blk, zigzag_blk);
    blk_print("zigzag", zigzag_blk, BLKID);

    // run-length
    xRLETable tbl = rtb_calloc(N * N);
    int tbl_len = run_length(tbl, zigzag_blk);
    rtb_print(tbl, tbl_len);

    // huffman

    printf("\n========decoding========\n");

    // normalize DCT for showing
    mat_foreach_blk(dct_mat, N, normalize_block, NULL);
    mat_get_blk(dct_mat, blk, BLKID);
    blk_print("normalized dct", blk, BLKID);

    // inverse DCT
    mat_foreach_blk(idct_mat, N, quantize_block, NULL);
    mat_foreach_blk(idct_mat, N, dequantize_block, NULL);

    mat_idct_blks(idct_mat, N);
    mat_get_blk(idct_mat, blk, BLKID);
    blk_print("idct", blk, BLKID);

    // rescale iDCT for showing
    mat_foreach_blk(idct_mat, N, rescale_block, NULL);
    mat_get_blk(idct_mat, blk, BLKID);
    blk_print("rescaled idct", blk, BLKID);

    // right shift 128
    mat_foreach_blk(idct_mat, N, rshift128_block, NULL);
    mat_get_blk(idct_mat, blk, BLKID);
    blk_print("rshift rescaled idct", blk, BLKID);

    mat_diff(orig_mat, idct_mat, diff_mat);
    mat_get_blk(diff_mat, blk, BLKID);
    blk_print("diff idct", blk, BLKID);

    // copy back to uint8 buffer for showing
    float_to_uint8_t(dctplane->buf, dct_mat, w * h);
    float_to_uint8_t(idctplane->buf, idct_mat, w * h);
    float_to_uint8_t(diffplane->buf, diff_mat, w * h);

    // draw_raster(yplane->buf, w, h);

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
