#ifdef ZOD_NGINE_IMPLEMENTATION
#include "../../../modules/log.h"

#include "engine_context_internal.h"

void engine_context_destroy(void) {
    log_debug("engine context: destroying...");
    cvar_destroy(&g_ctx.config.cvars);
    file_watcher_close(g_ctx.config.config_file_watcher);
}

engine_context g_ctx = {0};

#endif
