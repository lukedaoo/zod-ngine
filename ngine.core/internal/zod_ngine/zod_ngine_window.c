#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

int zngine_window_width(void) { return g_ctx.window.width; }
int zngine_window_height(void) { return g_ctx.window.height; }

#endif
