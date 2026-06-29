#ifdef ZOD_NGINE_IMPLEMENTATION

#include <glad/gl.h>
#include <stdlib.h>

#include <modules/log.h>
#include "window_internal.h"
#include "../../zod_ngine.h"

static bool window_gl_init(Window *window) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    window->gl_ctx = SDL_GL_CreateContext(window->handle);
    if (!window->gl_ctx) return false;
    return gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) != 0;
}

static void window_gl_destroy(Window *window) {
    if (window->gl_ctx) SDL_GL_DestroyContext(window->gl_ctx);
    window->gl_ctx = NULL;
}

Window window_create(const char *title, int width, int height, uint32_t flags) {
    Window window = {0};
    window.handle = SDL_CreateWindow(title, width, height, flags);
    window.width  = width;
    window.height = height;
    window_gl_init(&window);
    return window;
}

void window_destroy(Window *window) {
    if (!window) return;
    window_gl_destroy(window);
    SDL_DestroyWindow(window->handle);
}

bool window_apply_config(Window *window) {
    if (!window || !window->gl_ctx) return false;
    bool ok    = true;
    bool vsync = config_get_bool("window.vsync", true);
    if (!SDL_GL_SetSwapInterval(vsync ? 1 : 0) != 0) {
        log_warn("window.apply_config: vsync=%d set failed — %s, continuing without vsync",
                 vsync ? 1 : 0, SDL_GetError());
        ok = false;
    }
    return ok;
}

#endif
