#ifdef ZOD_NGINE_IMPLEMENTATION
#include <modules/log.h>

#include "engine_context_internal.h"

void engine_context_destroy(void) {
    log_debug("engine context: destroying...");
    window_destroy(&g_ctx.window);
    config_destroy(&g_ctx.config);
}

engine_context g_ctx = {0};

#endif
