#ifdef ZOD_NGINE_IMPLEMENTATION
#include "../engine_context.h"
#include "../../modules/cvar.h"

struct engine_context {
    cvar_table *config;
};

engine_context g_core = {0};

#endif
