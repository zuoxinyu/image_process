#include <SDL2/SDL.h>

#include "src/pxb.h"

#define OPAQUE_NAME "previewwindow"
#define WIN_SIZE_FACTOR 1
#define WIN_SCALE_FACTOR 0.25

typedef struct {
    int scale_factor;
} WindowState;

typedef struct PreviewWindow {
    const char *title;
    PixelBuffer *pxb;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} PreviewWindow;

PreviewWindow *create_preview_window(const char *title, PixelBuffer *pxb);
void destroy_preview_window(PreviewWindow *win);
