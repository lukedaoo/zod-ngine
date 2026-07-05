#ifndef INPUT_INTERNAL_H
#define INPUT_INTERNAL_H

#include <SDL3/SDL_scancode.h>
#include <stdbool.h>

struct input {
    bool curr[SDL_SCANCODE_COUNT];
    bool prev[SDL_SCANCODE_COUNT];
};

#endif
