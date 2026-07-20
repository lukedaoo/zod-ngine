#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/log.h>
#include <ngine.lib/cvar_load.h>

#include "config_internal.h"

#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

static const cvar_constraint g_engine_constraints[] = {
     {.name     = "engine.target_fps",
      .expected = CVAR_INT,
      .range    = {.has_min = true, .min.i = 0}},

     {.name     = "window.width",
      .expected = CVAR_INT,
      .range    = {.has_min = true, .min.i = 1}},
     {.name     = "window.height",
      .expected = CVAR_INT,
      .range    = {.has_min = true, .min.i = 1}},
     {.name = "window.title", .expected = CVAR_STRING},
     {.name = "window.vsync", .expected = CVAR_BOOL},
     {.name = "window.clear_color", .expected = CVAR_INT},
     {.name = "window.transparent", .expected = CVAR_BOOL},
};

#define ENGINE_CONSTRAINTS_COUNT 7

void config_seed_preset(config *cfg) {
    cvar_set_int(&cfg->cvars, "engine.target_fps", DEFAULT_CONFIG_TARGET_FPS);

    cvar_set_int(&cfg->cvars, "window.width", DEFAULT_CONFIG_WINDOW_WIDTH);
    cvar_set_int(&cfg->cvars, "window.height", DEFAULT_CONFIG_WINDOW_HEIGHT);
    cvar_set_string(&cfg->cvars, "window.title", DEFAULT_CONFIG_WINDOW_TITLE);
    cvar_set_bool(&cfg->cvars, "window.vsync", DEFAULT_CONFIG_WINDOW_VSYNC);
    cvar_set_int(&cfg->cvars, "window.clear_color", DEFAULT_CONFIG_WINDOW_CLEAR_COLOR);
    cvar_set_bool(&cfg->cvars, "window.transparent", DEFAULT_CONFIG_WINDOW_TRANSPARENT);

    cvar_set_int(&cfg->cvars, "log.level", DEFAULT_CONFIG_LOG_LEVEL);
}

void config_init(config *cfg) {
    log_debug("config.init: seeding defaults");
    config_seed_preset(cfg);
    cvar_add_schema(&cfg->cvars, g_engine_constraints, ENGINE_CONSTRAINTS_COUNT);
}

void config_destroy(config *cfg) {
    if (!cfg) return;
    cvar_destroy(&cfg->cvars);
    file_watcher_close(cfg->config_file_watcher);
}

void config_add_user_constraints(config *cfg, const cvar_constraint *entries,
                                 size_t count) {
    cvar_add_schema(&cfg->cvars, entries, count);
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

    bool ok = false;
    if (strcmp(ext, ".scf") == 0) {
        ok = cvar_load_scf(cvars, filepath, false);
    } else if (strcmp(ext, ".ini") == 0) {
        ok = cvar_load_ini(cvars, filepath, false);
    } else {
        log_warn("config.load: unsupported extension '%s' in '%s' — use .scf or .ini",
                 ext, filepath);
    }
    return ok;
}

bool config_reload_from_file(config *cfg) {
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

    config tmp = {0};
    config_seed_preset(&tmp);
    zod_run_extension_init_config(&tmp.cvars);
    cvar_copy_schema(&tmp.cvars, &cfg->cvars);
    if (!cfg->reload_config_func(cfg->config_file_watcher->path, &tmp.cvars)) {
        cvar_destroy(&tmp.cvars);
        log_warn("config.reload: failed to reload '%s' — keeping previous config",
                 cfg->config_file_watcher->path);
        return false;
    }

    if (!config_validate(&tmp)) {
        cvar_destroy(&tmp.cvars);
        log_warn("config.reload: reloaded config failed validation — keeping previous "
                 "config");
        return false;
    }

    cvar_destroy(&cfg->cvars);
    cfg->cvars = tmp.cvars;
    return true;
}

bool config_adjust(config *cfg) {
    if (!cfg) {
        log_error("config.adjust: called with NULL cfg — this is a bug");
        return false;
    }
    {
        cvar *log_level = cvar_get(&cfg->cvars, "log.level");
        if (log_level && log_level->type == CVAR_STRING) {
            int level_as_int = log_level_from_string(log_level->value.str.data);
            if (level_as_int < 0) {
                log_error("config.adjust: invalid log.level '%s'",
                          log_level->value.str.data);
                return false;
            }
            log_debug("config.adjust: log.level. string '%s' to int '%d'",
                      log_level->value.str.data, level_as_int);
            cvar_set_int(&cfg->cvars, "log.level", level_as_int);
        }
    }
    return true;
}

bool config_validate(config *cfg) {
    if (!cfg) {
        log_error("config.validate: called with NULL cfg — this is a bug");
        return false;
    }

    int w = cvar_get_int(&cfg->cvars, "window.width", DEFAULT_CONFIG_WINDOW_WIDTH);
    int h = cvar_get_int(&cfg->cvars, "window.height", DEFAULT_CONFIG_WINDOW_HEIGHT);
    int target_fps =
         cvar_get_int(&cfg->cvars, "engine.target_fps", DEFAULT_CONFIG_TARGET_FPS);

    if (w <= 0 || h <= 0) {
        log_fatal("config.validate: invalid config — window %dx%d — must be > 0", w, h);
        return false;
    }
    if (target_fps < 0) {
        log_fatal(
             "config.validate: invalid config — target_fps %d — must be >= 0 (0 = "
             "uncapped)",
             target_fps);
        return false;
    }

    return true;
}

void config_print(config *cfg) {
    log_debug("config.print: current values");
    cvar_print(&cfg->cvars);
}

#endif
