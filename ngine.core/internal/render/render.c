#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../window/window_internal.h"
#include "../engine_context/engine_context_internal.h"

void render_priv_begin(void) { render_backend_priv_begin(g_ctx.window.backend.context); }

void render_priv_end(void) { render_backend_priv_end(g_ctx.window.backend.context); }

#endif
