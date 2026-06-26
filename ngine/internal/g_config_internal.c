#ifdef ZOD_NGINE_IMPLEMENTATION
#include "../../modules/types.h"
#include "../g_config.h"

struct g_config {
    u16   window_width;
    u16   window_height;
    char  title[256];
    bool  vsync;
    u8    log_level;
    void *user;
};

static const g_config g_config_preset = {
     .window_width  = 800,
     .window_height = 600,
     .title         = "zod-ngine",
     .vsync         = true,
     .log_level     = 0,
     .user          = NULL,
};

static void g_config_register_preset(cvar_table *cvars) {
    cvar_set_int(cvars, "window.width", g_config_preset.window_width);
    cvar_set_int(cvars, "window.height", g_config_preset.window_height);
    cvar_set_string(cvars, "window.title", g_config_preset.title);
    cvar_set_bool(cvars, "window.vsync", g_config_preset.vsync);
    cvar_set_int(cvars, "log.level", g_config_preset.log_level);
}

static g_config g_config_storage = {0};

#endif
