#ifdef ZOD_NGINE_IMPLEMENTATION

#include "config_internal.h"
#include "../../../modules/log.h"
#include "../../../modules/cvar_load.h"

void g_config_seed_preset(g_config *cfg) {
    cvar_set_int(&cfg->cvars, "window.width", 800);
    cvar_set_int(&cfg->cvars, "window.height", 600);
    cvar_set_string(&cfg->cvars, "window.title", "zod-ngine");
    cvar_set_bool(&cfg->cvars, "window.vsync", true);
    cvar_set_int(&cfg->cvars, "log.level", 0);
}

static bool load_config_from_file_default(const char *filepath, cvar_table *cvars) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        log_debug("config: no file extension in path: %s", filepath);
        return false;
    }
    if (strcmp(ext, ".scf") == 0) {
        return cvar_load_scf(cvars, filepath, cvar_default_config_parser_handler, false);
    }
    if (strcmp(ext, ".ini") == 0) {
        return cvar_load_ini(cvars, filepath, cvar_default_config_parser_handler, false);
    }
    log_debug("config: unsupported file extension: %s", ext);
    return false;
}

bool g_reload_config_from_file(g_config *cfg) {
    if (!cfg || !cfg->config_file_watcher) {
        log_warn("config: no file watcher");
        return false;
    }
    if (!load_config_from_file_default(cfg->config_file_watcher->path, &cfg->cvars)) {
        log_warn("failed to load config file");
        return false;
    }
    return true;
}

g_config g_config_storage = {0};

#endif
