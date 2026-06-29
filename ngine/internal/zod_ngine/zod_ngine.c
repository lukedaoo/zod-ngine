#ifdef ZOD_NGINE_IMPLEMENTATION
#include <unistd.h>
#include <SDL3/SDL.h>

#include <modules/cvar.h>
#include <modules/log.h>
#include <modules/cvar_load.h>
#include <modules/file_watcher.h>

#include "../../config.h"
#include "../../render.h"
#include "../../zod_ngine.h"

#include "../config/config_internal.h"
#include "../clock/clock_internal.h"
#include "../engine_context/engine_context_internal.h"

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                     argc        = params.argc;
    const char                  **argv        = params.argv;
    const zod_config_file_setup_t config_file = params.config_file_setup;
    const zod_engine_dispatch     dispatch    = params.dispatch;

    if (dispatch.before_init) {
        dispatch.before_init();
    }

    log_info("zod_ngine: init");

    {
        log_debug("config: initializing...");

        g_config_seed_preset(&g_ctx.config);
        log_debug("config: preset loaded");

        if (config_file.load_config_func && config_file.config_path) {
            if (!config_file.load_config_func(config_file.config_path,
                                              &g_ctx.config.cvars)) {
                log_warn("config: file load failed (%s) — using preset",
                         config_file.config_path);
            } else {
                log_debug("config: loaded from %s", config_file.config_path);
                if (config_file.hot_reload) {
                    g_ctx.config.config_file_watcher =
                         file_watcher_watch(config_file.config_path);
                    g_ctx.config.reload_config_func = config_file.load_config_func;
                    log_debug("config: hot reload enabled");
                }
            }
        }

        if (dispatch.load_args) {
            if (!dispatch.load_args(argc, argv, &g_ctx.config.cvars)) {
                log_warn("config: args parse failed — some overrides ignored");
            } else {
                log_debug("config: args applied");
            }
        }
        g_adjust_config(&g_ctx.config);
    }

#ifdef DEBUG
    log_debug("config: printing...");
    g_config_print(&g_ctx.config);
#endif

    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            log_info("SDL: initialized failed");
            return false;
        }
    }
    //
    // window initialization
    //
    {
        const char *title = config_get_string("window.title", "zod-ngine");
        int         w     = config_get_int("window.width", 800);
        int         h     = config_get_int("window.height", 600);
        g_ctx.window      = window_create(title, w, h, SDL_WINDOW_OPENGL);
        zod_ngine_apply_config();
    }

    {
        log_debug("clock: setting up clock...");
        const int target_fps = config_get_int("engine.target_fps", 60);
        g_clock_init(target_fps);
    }

    if (dispatch.after_init) {
        dispatch.after_init();
    }

    log_info("zod_ngine: ready");
    return true;
}

void zod_ngine_destroy(void) {
    log_debug("zod_ngine: destroying engine...");
    engine_context_destroy();
}

void zod_ngine_apply_config(void) {
    log_set_level(config_get_int("log.level", LOG_TRACE));
    window_apply_config(&g_ctx.window);
}

int config_get_int(const char *name, int fallback) {
    return cvar_get_int(&g_ctx.config.cvars, name, fallback);
}

float config_get_float(const char *name, float fallback) {
    return cvar_get_float(&g_ctx.config.cvars, name, fallback);
}

bool config_get_bool(const char *name, bool fallback) {
    return cvar_get_bool(&g_ctx.config.cvars, name, fallback);
}

const char *config_get_string(const char *name, const char *fallback) {
    return cvar_get_string(&g_ctx.config.cvars, name, fallback);
}

bool config_set_int(const char *name, int value) {
    return cvar_set_int(&g_ctx.config.cvars, name, value);
}
bool config_set_float(const char *name, float value) {
    return cvar_set_float(&g_ctx.config.cvars, name, value);
}
bool config_set_bool(const char *name, bool value) {
    return cvar_set_bool(&g_ctx.config.cvars, name, value);
}

bool config_set_string(const char *name, const char *value) {
    return cvar_set_string(&g_ctx.config.cvars, name, value);
}

float    clock_dt(void) { return g_ctx.clock.dt; }
float    clock_delta(void) { return g_ctx.clock.delta; }
float    clock_now(void) { return g_ctx.clock.now; }
float    clock_frame_time(void) { return g_ctx.clock.now - g_ctx.clock.frame_last; }
uint32_t clock_frame(void) { return g_ctx.clock.frame_count; }
bool     clock_paused(void) { return g_ctx.clock.paused; }

void clock_set_time_scale(float scale) { g_ctx.clock.time_scale = scale; }
void clock_set_paused(bool paused) { g_ctx.clock.paused = paused; }

void zod_begin_drawing(void) { render_begin(); }
void zod_end_drawing(void) { render_end(&g_ctx.window); }

#endif
