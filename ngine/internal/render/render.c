#ifdef ZOD_NGINE_IMPLEMENTATION

#include <glad/gl.h>

#include "../../window.h"

#include "../window/window_internal.h"

// @todo:
// - setup default value
// - change with config file
void render_begin(void) {
    glClearColor(0.08F, 0.1F, 0.1F, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_end(const Window *win) { SDL_GL_SwapWindow(win->handle); }
#endif
