#ifdef ZOD_NGINE_IMPLEMENTATION
#include <ngine.lib/log.h>

#include "engine_context_internal.h"

void engine_context_priv_destroy(void) {
    log_debug("engine context: destroying...");
    window_priv_destroy(&g_ctx.window);
    config_priv_destroy(&g_ctx.config);
    cmd_manager_priv_destroy(&g_ctx.cmd_manager);
}

engine_context g_ctx = {0};

#endif
