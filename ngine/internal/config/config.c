#ifdef ZOD_NGINE_IMPLEMENTATION

#include "config_internal.h"

void g_config_seed_preset(g_config *cfg) {
    cvar_set_int(&cfg->cvars, "window.width", 800);
    cvar_set_int(&cfg->cvars, "window.height", 600);
    cvar_set_string(&cfg->cvars, "window.title", "zod-ngine");
    cvar_set_bool(&cfg->cvars, "window.vsync", true);
    cvar_set_int(&cfg->cvars, "log.level", 0);
}

g_config g_config_storage = {0};

#endif
