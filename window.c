#include "window.h"

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

    win->title = title;
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
