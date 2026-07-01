#ifdef ZOD_NGINE_IMPLEMENTATION

#include <string.h>
#include <SDL3/SDL.h>

#include "../../input.h"
#include "../engine_context/engine_context_internal.h"
#include "input_internal.h"

void zod_input_update(void) {
    memcpy(g_ctx.input.prev, g_ctx.input.curr, sizeof(g_ctx.input.prev));

    int         num_keys = 0;
    const bool *state    = SDL_GetKeyboardState(&num_keys);

    size_t n =
         (size_t)num_keys < SDL_SCANCODE_COUNT ? (size_t)num_keys : SDL_SCANCODE_COUNT;
    memcpy(g_ctx.input.curr, state, n * sizeof(bool));
}

bool zod_key_down(zod_key_t key) {
    return (key < SDL_SCANCODE_COUNT) ? g_ctx.input.curr[key] : false;
}

bool zod_key_pressed(zod_key_t key) {
    return (key < SDL_SCANCODE_COUNT)
               ? (g_ctx.input.curr[key] && !g_ctx.input.prev[key])
               : false;
}

bool zod_key_released(zod_key_t key) {
    return (key < SDL_SCANCODE_COUNT)
               ? (!g_ctx.input.curr[key] && g_ctx.input.prev[key])
               : false;
}

#endif
