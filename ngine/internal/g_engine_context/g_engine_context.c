#ifdef ZOD_NGINE_IMPLEMENTATION
#include "ngine/g_engine_context.h"
#include "ngine/g_config.h"

struct engine_context {
    g_config *config;
};

engine_context g_ctx = {0};

#endif
