#ifdef ZOD_NGINE_IMPLEMENTATION
#include <unistd.h>
#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <modules/cvar.h>
#include <modules/log.h>
#include <modules/cvar_load.h>
#include <modules/file_watcher.h>

#include "../../config.h"
#include "../../render.h"
#include "../../zod_error.h"
#include "../../zod_ngine.h"

#include "../config/config_internal.h"
#include "../clock/clock_internal.h"
#include "../engine_context/engine_context_internal.h"

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                 argc         = params.argc;
    const char              **argv         = params.argv;
    const zod_config_setup_t  config_setup = params.config_setup;
    const zod_engine_dispatch dispatch     = params.dispatch;

    if (dispatch.before_init) {
        dispatch.before_init();
    }

    log_info("engine.init: starting");

    {
        log_debug("config.init: seeding defaults");
        g_config_seed_preset(&g_ctx.config);
        g_ctx.config.user_schema = config_setup.schema;

        if (config_setup.load_config_func && config_setup.config_path) {
            if (!config_setup.load_config_func(config_setup.config_path,
                                               &g_ctx.config.cvars)) {
                log_warn("config.init: failed to load '%s' — using preset defaults",
                         config_setup.config_path);
            } else {
                log_debug("config.init: loaded from '%s'", config_setup.config_path);
            }
            if (config_setup.hot_reload) {
                g_ctx.config.config_file_watcher =
                     file_watcher_watch(config_setup.config_path);
                g_ctx.config.reload_config_func = config_setup.load_config_func;
                log_debug("config.init: hot reload enabled for '%s'",
                          config_setup.config_path);
            }
        }

        if (dispatch.load_args) {
            if (!dispatch.load_args(argc, argv, &g_ctx.config.cvars)) {
                log_warn(
                     "config.init: CLI args parse failed — command-line overrides "
                     "ignored");
            } else {
                log_debug("config.init: CLI args applied");
            }
        }
        g_adjust_config(&g_ctx.config);
    }

#ifdef DEBUG
    g_config_print(&g_ctx.config);
#endif

#ifndef NGINE_UNIT_TEST
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
            zod_set_error("SDL_Init failed: %s", SDL_GetError());
            log_fatal("engine.init: SDL_Init failed — %s, check SDL installation",
                      SDL_GetError());
            return false;
        }
    }
    {
        const char *title = config_get_string("window.title", "zod-ngine");
        int         w     = config_get_int("window.width", 800);
        int         h     = config_get_int("window.height", 600);
        // @Robustness: remove hard-coded OpenGL dependency; support backend selection
        uint32_t flags = SDL_WINDOW_OPENGL;
        if (config_get_bool("window.transparent", false)) flags |= SDL_WINDOW_TRANSPARENT;
        g_ctx.window = window_create(title, w, h, flags);
        zod_ngine_apply_config(false);
    }
#endif

    {
        const int target_fps = config_get_int("engine.target_fps", 60);
        g_clock_init(target_fps);
        log_debug("clock.init: target fps = %d", target_fps);
    }

    if (dispatch.after_init) {
        dispatch.after_init();
    }

    log_info("engine.init: ready");
    return true;
}

void zod_ngine_destroy(void) {
    log_debug("engine.destroy: shutting down");
    engine_context_destroy();
}

void zod_ngine_apply_config(bool adjust_config) {
    if (adjust_config) {
        g_adjust_config(&g_ctx.config);
    }
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
