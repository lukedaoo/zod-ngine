#ifdef ZOD_NGINE_IMPLEMENTATION
#include "../../../modules/cvar.h"
#include "../../../modules/log.h"
#include "../../../modules/cvar_load.h"
#include "../../../modules/file_watcher.h"

#include "../../zod_ngine.h"
#include "../config/config_internal.h"
#include "../engine_context/engine_context_internal.h"
#include <unistd.h>

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                     argc        = params.argc;
    const char                  **argv        = params.argv;
    const zod_config_file_setup_t config_file = params.config_file_setup;
    const zod_engine_dispatch     dispatch    = params.dispatch;

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
        if (!config_file.config_path) {
            log_debug("no config file path provided");
            return false;
        }
        log_debug("loading config file from path: %s", config_file.config_path);
        if (!dispatch.load_config_from_file(config_file.config_path,
                                            &g_config_storage.cvars)) {
            log_debug("config file load failed, keeping preset values");
            if (config_file.hot_reload) {
                log_debug("config file watcher enabled");
                g_config_storage.config_file_watcher =
                     file_watcher_watch(config_file.config_path);
            }
        }
    }

    //
    // Stage 3: CLI overlay
    //
    if (dispatch.load_args) {
        log_debug("parsing command line...");
        if (!dispatch.load_args(argc, argv, &g_config_storage.cvars)) {
            log_debug("failed to parse command line, some args are invalid");
        }
    }

    g_ctx.config = &g_config_storage;

    if (dispatch.after_init) {
        dispatch.after_init();
    }

    return true;
}

void zod_ngine_destroy(void) {
    log_debug("destroying engine...");
    if (g_ctx.config) {
        file_watcher_close(g_ctx.config->config_file_watcher);
        cvar_destroy(&g_ctx.config->cvars);
    }
}

void main_loop(void) {
    while (true) {
        if (g_ctx.config->config_file_watcher) {
            file_status status = file_watcher_check(g_ctx.config->config_file_watcher);
            if (status == FILE_CHANGED) {
                log_info("config file changed");
                if (!g_reload_config_from_file(g_ctx.config)) {
                    log_warn("failed to load config file");
                }
            }
        }
        // @hack: this is bad. Just make the file watcher works
        log_info("main loop: config %s",
                 config_get_string("client.password", "zod-ngine"));
        sleep(2);
    }
}

int config_get_int(const char *name, int fallback) {
    return cvar_get_int(&g_config_storage.cvars, name, fallback);
}

float config_get_float(const char *name, float fallback) {
    return cvar_get_float(&g_config_storage.cvars, name, fallback);
}

bool config_get_bool(const char *name, bool fallback) {
    return cvar_get_bool(&g_config_storage.cvars, name, fallback);
}

const char *config_get_string(const char *name, const char *fallback) {
    return cvar_get_string(&g_config_storage.cvars, name, fallback);
}

bool config_set_int(const char *name, int value) {
    return cvar_set_int(&g_config_storage.cvars, name, value);
}
bool config_set_float(const char *name, float value) {
    return cvar_set_float(&g_config_storage.cvars, name, value);
}
bool config_set_bool(const char *name, bool value) {
    return cvar_set_bool(&g_config_storage.cvars, name, value);
}

bool config_set_string(const char *name, const char *value) {
    return cvar_set_string(&g_config_storage.cvars, name, value);
}

#endif
