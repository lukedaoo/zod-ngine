#ifdef ZOD_NGINE_IMPLEMENTATION

#include <modules/log.h>
#include <modules/cvar_load.h>

#include "config_internal.h"

#ifndef DEFAULT_CONFIG_TARGET_FPS
#define DEFAULT_CONFIG_TARGET_FPS 60
#endif

#ifndef DEFAULT_CONFIG_WINDOW_WIDTH
#define DEFAULT_CONFIG_WINDOW_WIDTH 800
#endif

#ifndef DEFAULT_CONFIG_WINDOW_HEIGHT
#define DEFAULT_CONFIG_WINDOW_HEIGHT 600
#endif

#ifndef DEFAULT_CONFIG_WINDOW_TITLE
#define DEFAULT_CONFIG_WINDOW_TITLE "zod-ngine"
#endif

#ifndef DEFAULT_CONFIG_WINDOW_VSYNC
#define DEFAULT_CONFIG_WINDOW_VSYNC true
#endif

#ifndef DEFAULT_CONFIG_LOG_LEVEL
// LOG_TRACE = 0,
// LOG_DEBUG = 1,
// LOG_INFO = 2,
// LOG_WARN = 3,
// LOG_ERROR = 4,
// LOG_FATAL = 5
#define DEFAULT_CONFIG_LOG_LEVEL 0
#endif

void g_config_seed_preset(g_config *cfg) {
    cvar_set_int(&cfg->cvars, "engine.target_fps", DEFAULT_CONFIG_TARGET_FPS);

    cvar_set_int(&cfg->cvars, "window.width", DEFAULT_CONFIG_WINDOW_WIDTH);
    cvar_set_int(&cfg->cvars, "window.height", DEFAULT_CONFIG_WINDOW_HEIGHT);
    cvar_set_string(&cfg->cvars, "window.title", DEFAULT_CONFIG_WINDOW_TITLE);
    cvar_set_bool(&cfg->cvars, "window.vsync", DEFAULT_CONFIG_WINDOW_VSYNC);

    cvar_set_int(&cfg->cvars, "log.level", DEFAULT_CONFIG_LOG_LEVEL);
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

bool g_config_reload_from_file(g_config *cfg) {
    if (!cfg || !cfg->config_file_watcher) {
        log_warn("config: no file watcher");
        return false;
    }

    if (!cfg->reload_config_func) {
        log_warn("config: no reload function. Try to use default");
        cfg->reload_config_func = load_config_from_file_default;
    }

    if (!cfg->reload_config_func(cfg->config_file_watcher->path, &cfg->cvars)) {
        log_warn("failed to load config file");
        return false;
    }
    return true;
}

bool g_adjust_config(g_config *cfg) {
    if (!cfg) {
        log_warn("config: no config to adjust");
        return false;
    }
    {
        log_debug("config: converting log.level to int if necessary");
        cvar_t *log_level = cvar_get(&cfg->cvars, "log.level");
        if (log_level && log_level->type == CVAR_STRING) {
            int level_as_int = log_level_from_string(log_level->value.str.data);
            cvar_set_int(&cfg->cvars, "log.level", level_as_int);
        }
    }
    return true;
}

void g_config_print(g_config *cfg) {
    log_debug("config: print:");
    cvar_print(&cfg->cvars);
}

#endif
