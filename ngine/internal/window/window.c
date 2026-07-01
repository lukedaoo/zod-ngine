#ifdef ZOD_NGINE_IMPLEMENTATION

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#include <glad/gl.h>
#endif

#include <modules/log.h>

#include "../../window.h"
#include "../../common.h"

#include "window_internal.h"
#include "../config/config_internal.h"
#include "../engine_context/engine_context_internal.h"
#include "../render/render_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#define WINDOW_SDL_FLAG SDL_WINDOW_OPENGL
#elif RENDER_BACKEND == RENDER_BACKEND_VULKAN
#define WINDOW_SDL_FLAG SDL_WINDOW_VULKAN
#endif

window window_create(const char *title, int width, int height, uint32_t flags) {
    window window = {0};
    window.handle = SDL_CreateWindow(title, width, height, flags | WINDOW_SDL_FLAG);
    window.width  = width;
    window.height = height;
    log_info("window.create: title='%s' size=%dx%d, backend=%s", title, width, height,
             RENDER_BACKEND == RENDER_BACKEND_OPENGL ? "opengl" : "vulkan");
    window.backend.type    = RENDER_BACKEND;
    window.backend.context = render_backend_context_create(window.handle);
    render_backend_init(window.backend.context, width, height);
    return window;
}

void window_destroy(window *window) {
    if (!window) return;
    render_backend_shutdown(window->backend.context);
    SDL_DestroyWindow(window->handle);
}

bool window_apply_config(window *window) {
    if (!window || !window->backend.context) return false;

    bool ok = true;
    //
    // vsync
    //
    {
#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
        bool vsync    = cvar_get_bool(&g_ctx.config.cvars, "window.vsync", true);
        bool vsync_ok = SDL_GL_SetSwapInterval(vsync ? 1 : 0);
        if (!vsync_ok) {
            log_warn(
                 "window.apply_config: vsync=%d set failed — %s, continuing without "
                 "vsync",
                 vsync ? 1 : 0, SDL_GetError());
            ok = false;
        }
#endif
    }
    //
    // window size
    //
    {
        int w = cvar_get_int(&g_ctx.config.cvars, "window.width", 800);
        int h = cvar_get_int(&g_ctx.config.cvars, "window.height", 600);
        if (w != window->width || h != window->height) {
            SDL_SetWindowSize(window->handle, w, h);
            window->width  = w;
            window->height = h;
            render_backend_resize(window->backend.context, w, h);
            log_debug("window.apply_config: resized to %dx%d", w, h);
        }
    }
    //
    // clear color
    //
    {
        uint32_t clear_color =
             (uint32_t)cvar_get_int(&g_ctx.config.cvars, "window.clear_color",
                                    (int)DEFAULT_CONFIG_WINDOW_CLEAR_COLOR);
        if (clear_color != window->clear_color) {
            window->clear_color = clear_color;
#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
            const color4f c = color4f_from_u32(clear_color);
            glClearColor(c.r, c.g, c.b, c.a);
#endif
        }
    }
    return ok;
}

#endif
