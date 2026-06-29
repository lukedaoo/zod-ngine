#ifndef WINDOW_INTERNAL_H
#define WINDOW_INTERNAL_H

#include <SDL3/SDL.h>
#include "modules/types.h"

struct Window {
    SDL_Window   *handle;
    SDL_GLContext gl_ctx;
    int           width;
    int           height;
    uint32_t      clear_color;  // 0xRRGGBBAA
};

#endif
