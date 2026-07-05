#ifndef WINDOW_INTERNAL_H
#define WINDOW_INTERNAL_H

#include <SDL3/SDL.h>
#include "modules/types.h"
#include "../render/render_internal.h"

struct g_window {
    SDL_Window    *handle;
    render_backend backend;
    int            width;
    int            height;
    uint32_t       clear_color;  // 0xRRGGBBAA
};

#endif
