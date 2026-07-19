#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../window/window_internal.h"
#include "../engine_context/engine_context_internal.h"

void render_begin(void) { render_backend_begin(g_ctx.window.backend.context); }

void render_end(void) { render_backend_end(g_ctx.window.backend.context); }

#endif
