#ifdef ZOD_NGINE_IMPLEMENTATION
#include <stdio.h>
#include <string.h>

#include "modules/carg.h"
#include "modules/cvar.h"
#include "modules/cvar_load.h"
#include "modules/log.h"

#include "ngine/g_engine_context.h"
#include "ngine/zod_ngine.h"

struct zod_engine_dispatch {
    void (*before_init)(void);
    bool (*load_config_from_file)(const char *filepath, cvar_table *cvars);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
};

struct zod_engine_init_params {
    int                 argc;
    const char        **argv;
    const char         *config_path;
    zod_engine_dispatch dispatch;
};

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                 argc     = params.argc;
    const char              **argv     = params.argv;
    const zod_engine_dispatch dispatch = params.dispatch;

    if (dispatch.before_init) {
        dispatch.before_init();
    }

    log_debug("initializing engine...");

    //
    // Stage 1: seed preset defaults into embedded cvars
    //
    g_config_seed_preset(&g_config_storage);

    //
    // Stage 2: file overlay
    //
    if (dispatch.load_config_from_file) {
        log_debug("loading config file");
        if (!dispatch.load_config_from_file(params.config_path,
                                            &g_config_storage.cvars)) {
            log_debug("config file load failed, keeping preset values");
        }
    }

    //
    // Stage 3: CLI overlay
    //
    if (dispatch.load_args) {
        log_debug("parsing command line...");
        if (!dispatch.load_args(argc, argv, &g_config_storage.cvars)) {
            log_debug("failed to parse command line");
        }
    }

    g_ctx.config = &g_config_storage;

    if (dispatch.after_init) {
        dispatch.after_init();
    }

    return true;
}

void zod_ngine_destroy(void) { log_debug("destroying engine..."); }

int get_int_config(const char *name, int fallback) {
    return cvar_get_int(&g_config_storage.cvars, name, fallback);
}

float get_float_config(const char *name, float fallback) {
    return cvar_get_float(&g_config_storage.cvars, name, fallback);
}

bool get_bool_config(const char *name, bool fallback) {
    return cvar_get_bool(&g_config_storage.cvars, name, fallback);
}

const char *get_string_config(const char *name, const char *fallback) {
    return cvar_get_string(&g_config_storage.cvars, name, fallback);
}

#endif
