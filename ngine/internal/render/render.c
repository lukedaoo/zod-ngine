#ifdef ZOD_NGINE_IMPLEMENTATION

#include <glad/gl.h>

#include "../../window.h"
#include "../../common.h"

#include "../window/window_internal.h"
#include "../engine_context/engine_context_internal.h"

void render_begin(void) {
    const color4f clear_color = color4f_from_u32(g_ctx.window.clear_color);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_end(const Window *win) { SDL_GL_SwapWindow(win->handle); }

#endif
