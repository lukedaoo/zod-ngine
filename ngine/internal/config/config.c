#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdlib.h>

#include <modules/log.h>
#include <modules/cvar_load.h>

#include "config_internal.h"

#include "../engine_context/engine_context_internal.h"

static const cvar_schema_entry g_engine_schema_entries[] = {
     {"engine.target_fps", CVAR_INT},   {"window.width", CVAR_INT},
     {"window.height", CVAR_INT},       {"window.title", CVAR_STRING},
     {"window.vsync", CVAR_BOOL},       {"window.clear_color", CVAR_INT},
     {"window.transparent", CVAR_BOOL},
};

static const cvar_schema g_engine_schema = {
     .entries = g_engine_schema_entries,
     .count   = 7,
};

void g_config_seed_preset(g_config *cfg) {
    cvar_set_int(&cfg->cvars, "engine.target_fps", DEFAULT_CONFIG_TARGET_FPS);

    cvar_set_int(&cfg->cvars, "window.width", DEFAULT_CONFIG_WINDOW_WIDTH);
    cvar_set_int(&cfg->cvars, "window.height", DEFAULT_CONFIG_WINDOW_HEIGHT);
    cvar_set_string(&cfg->cvars, "window.title", DEFAULT_CONFIG_WINDOW_TITLE);
    cvar_set_bool(&cfg->cvars, "window.vsync", DEFAULT_CONFIG_WINDOW_VSYNC);
    cvar_set_int(&cfg->cvars, "window.clear_color", DEFAULT_CONFIG_WINDOW_CLEAR_COLOR);
    cvar_set_bool(&cfg->cvars, "window.transparent", DEFAULT_CONFIG_WINDOW_TRANSPARENT);

    cvar_set_int(&cfg->cvars, "log.level", DEFAULT_CONFIG_LOG_LEVEL);
}

static bool load_config_from_file_default(const char *filepath, cvar_table *cvars) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        log_warn(
             "config.load: '%s' has no file extension — cannot determine format, use "
             ".scf or .ini",
             filepath);
        return false;
    }

    size_t cap = g_engine_schema.count +
                 (g_ctx.config.user_schema ? g_ctx.config.user_schema->count : 0);
    cvar_schema_entry *merged_buf = malloc(cap * sizeof *merged_buf);

    size_t n =
         cvar_schema_merge(&g_engine_schema, g_ctx.config.user_schema, merged_buf, cap);
    cvar_schema schema = {.entries = merged_buf, .count = n};

    bool ok = false;
    if (strcmp(ext, ".scf") == 0) {
        ok = cvar_load_scf(cvars, filepath, &schema, false);
    } else if (strcmp(ext, ".ini") == 0) {
        ok = cvar_load_ini(cvars, filepath, &schema, false);
    } else {
        log_warn("config.load: unsupported extension '%s' in '%s' — use .scf or .ini",
                 ext, filepath);
    }
    free(merged_buf);
    return ok;
}

bool g_config_reload_from_file(g_config *cfg) {
    if (!cfg || !cfg->config_file_watcher) {
        log_error(
             "config.reload: called without a file watcher — enable hot_reload=true in "
             "zod_engine_init_params");
        return false;
    }

    if (!cfg->reload_config_func) {
        log_debug("config.reload: no reload func set, falling back to default loader");
        cfg->reload_config_func = load_config_from_file_default;
    }

    g_config tmp = {0};
    g_config_seed_preset(&tmp);
    if (!cfg->reload_config_func(cfg->config_file_watcher->path, &tmp.cvars)) {
        cvar_destroy(&tmp.cvars);
        log_warn("config.reload: failed to reload '%s' — keeping previous config",
                 cfg->config_file_watcher->path);
        return false;
    }

    cvar_destroy(&cfg->cvars);
    cfg->cvars = tmp.cvars;
    return true;
}

bool g_config_adjust(g_config *cfg) {
    if (!cfg) {
        log_error("config.adjust: called with NULL cfg — this is a bug");
        return false;
    }
    {
        cvar_t *log_level = cvar_get(&cfg->cvars, "log.level");
        if (log_level && log_level->type == CVAR_STRING) {
            int level_as_int = log_level_from_string(log_level->value.str.data);
            if (level_as_int < 0) {
                log_error("config.adjust: invalid log.level '%s'",
                          log_level->value.str.data);
                return false;
            }
            log_debug("config.adjust: log.level '%s' -> %d",
                      log_level->value.str.data, level_as_int);
            cvar_set_int(&cfg->cvars, "log.level", level_as_int);
        }
    }
    return true;
}

void g_config_print(g_config *cfg) {
    log_debug("config.print: current values");
    cvar_print(&cfg->cvars);
}

#endif
