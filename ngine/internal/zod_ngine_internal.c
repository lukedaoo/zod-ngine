#ifdef ZOD_NGINE_IMPLEMENTATION
#include <stdio.h>
#include <string.h>

#include "../../modules/carg.h"
#include "../../modules/cvar.h"
#include "../../modules/cvar_load.h"
#include "../../modules/log.h"

#include "../g_engine_context.h"
#include "../zod_ngine.h"

struct zod_engine_dispatch {
    void (*before_init)(void);
    bool (*load_config_from_file)(const char *filepath, cvar_table *merged);
    bool (*load_args)(int argc, const char **argv, cvar_table *merged);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
};

struct zod_engine_init_params {
    int                 argc;
    const char        **argv;
    const char         *config_path;
    void               *user_config;
    zod_engine_dispatch dispatch;
};

static void config_apply_merged(g_config *cfg, cvar_table *merged) {
    cfg->window_width  = cvar_get_int(merged, "window.width", cfg->window_width);
    cfg->window_height = cvar_get_int(merged, "window.height", cfg->window_height);
    cfg->vsync         = cvar_get_bool(merged, "window.vsync", cfg->vsync);
    cfg->log_level     = cvar_get_int(merged, "log.level", cfg->log_level);

    cvar_t *title_cv = cvar_get(merged, "window.title");
    if (title_cv && title_cv->type == CVAR_STRING) {
        strncpy(cfg->title, title_cv->value.str.data, sizeof(cfg->title) - 1);
        cfg->title[sizeof(cfg->title) - 1] = '\0';
    }
}

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                 argc     = params.argc;
    const char              **argv     = params.argv;
    const zod_engine_dispatch dispatch = params.dispatch;

    if (dispatch.before_init) {
        dispatch.before_init();
    }

    log_debug("initializing engine...");

    cvar_table merged = {0};

    //
    // Stage 1: preset
    //
    {
        log_debug("loading preset config");
        g_config_register_preset(&merged);
    }

    //
    // Stage 2: file overlay
    //
    if (dispatch.load_config_from_file) {
        log_debug("loading config file");
        if (!dispatch.load_config_from_file(params.config_path, &merged)) {
            log_debug("config file load failed, keeping preset values");
        }
    }

    //
    // Stage 3: CLI overlay
    //
    if (dispatch.load_args) {
        log_debug("parsing command line...");
        if (!dispatch.load_args(argc, argv, &merged)) {
            log_debug("failed to parse command line");
        }
    }

    //
    // Pump merged
    //
    g_config_storage      = g_config_preset;
    g_config_storage.user = params.user_config;
    config_apply_merged(&g_config_storage, &merged);
    cvar_destroy(&merged);
    g_ctx.config = &g_config_storage;

    if (dispatch.after_init) {
        dispatch.after_init();
    }

    return true;
}

void zod_ngine_destroy(void) { log_debug("destroying engine..."); }

#endif
