#ifdef ZOD_NGINE_IMPLEMENTATION
#include "ngine/g_config.h"

struct g_config {
    cvar_table cvars;
#ifdef G_CONFIG_USER_TYPE
    G_CONFIG_USER_TYPE user;
#endif
};

static void g_config_seed_preset(g_config *cfg) {
    cvar_set_int(&cfg->cvars, "window.width", 800);
    cvar_set_int(&cfg->cvars, "window.height", 600);
    cvar_set_string(&cfg->cvars, "window.title", "zod-ngine");
    cvar_set_bool(&cfg->cvars, "window.vsync", true);
    cvar_set_int(&cfg->cvars, "log.level", 0);
}

static g_config g_config_storage = {0};

#endif
