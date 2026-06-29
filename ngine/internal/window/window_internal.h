#ifndef WINDOW_INTERNAL_H
#define WINDOW_INTERNAL_H

#include <SDL3/SDL.h>

struct Window {
    SDL_Window   *handle;
    SDL_GLContext gl_ctx;
    int           width;
    int           height;
};

#endif
