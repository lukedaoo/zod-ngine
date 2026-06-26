#ifndef G_ENGINE_CONTEXT_INTERNAL_H
#define G_ENGINE_CONTEXT_INTERNAL_H

#include "../../g_engine_context.h"
#include "../g_config/g_config_internal.h"

struct engine_context {
    g_config *config;
};

extern engine_context g_ctx;

#endif
