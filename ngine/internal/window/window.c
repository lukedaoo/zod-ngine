#ifdef ZOD_NGINE_IMPLEMENTATION

#include <glad/gl.h>
#include <stdlib.h>

#include <modules/log.h>

#include "../../window.h"
#include "../../clock.h"
#include "../../zod_ngine.h"

#include "window_internal.h"
#include "../config/config_internal.h"

static bool window_gl_init(Window *window) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // alpha-capable framebuffer required for compositor to honor window transparency
    if (SDL_GetWindowFlags(window->handle) & SDL_WINDOW_TRANSPARENT) {
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    }
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

    bool ok = true;
    //
    // vsync
    //
    {
        bool vsync    = config_get_bool("window.vsync", true);
        bool vsync_ok = SDL_GL_SetSwapInterval(vsync ? 1 : 0);
        if (!vsync_ok) {
            log_warn(
                 "window.apply_config: vsync=%d set failed — %s, continuing without "
                 "vsync",
                 vsync ? 1 : 0, SDL_GetError());
            ok = false;
        }
    }
    //
    // window size
    //
    {
        int w = config_get_int("window.width", 800);
        int h = config_get_int("window.height", 600);
        if (w != window->width || h != window->height) {
            SDL_SetWindowSize(window->handle, w, h);
            window->width  = w;
            window->height = h;
            glViewport(0, 0, w, h);
            log_debug("window.apply_config: resized to %dx%d", w, h);
        }
    }
    //
    // clear color
    //
    {
#ifdef DEFAULT_CONFIG_WINDOW_CLEAR_COLOR
        window->clear_color = (uint32_t)config_get_int(
             "window.clear_color", (int)DEFAULT_CONFIG_WINDOW_CLEAR_COLOR);
#else
        window->clear_color =
             (uint32_t)config_get_int("window.clear_color", (int)0x141A1AFF);
#endif
    }
    return ok;
}

#endif
