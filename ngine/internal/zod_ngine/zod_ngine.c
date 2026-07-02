#ifdef ZOD_NGINE_IMPLEMENTATION

#include <SDL3/SDL.h>

#include <modules/cvar.h>
#include <modules/log.h>
#include <modules/cvar_load.h>
#include <modules/file_watcher.h>

#include "../../config.h"
#include "../../console.h"
#include "../../render.h"
#include "../../zod_error.h"
#include "../../zod_ngine.h"

#include "../config/config_internal.h"
#include "../clock/clock_internal.h"
#include "../engine_context/engine_context_internal.h"

bool zod_ngine_init(const zod_engine_init_params params) {
    const int                 argc         = params.argc;
    const char              **argv         = params.argv;
    const zod_config_setup    config_setup = params.config_setup;
    const zod_engine_dispatch dispatch     = params.dispatch;

    void *user_data = params.user_data;

    if (dispatch.before_init) dispatch.before_init(user_data);

    log_info("\nengine.init: starting");

    {
        log_debug("config.init: seeding defaults");
        g_config_seed_preset(&g_ctx.config);
        g_ctx.config.user_schema = config_setup.schema;

        if (config_setup.load_config_func && config_setup.config_path) {
            if (!config_setup.load_config_func(config_setup.config_path,
                                               &g_ctx.config.cvars)) {
                const char *reason = cvar_load_get_error();
                if (reason[0]) {
                    zod_set_error("failed to load config '%s': %s",
                                  config_setup.config_path, reason);
                    log_fatal("config.init: failed to load '%s': %s",
                              config_setup.config_path, reason);
                } else {
                    zod_set_error("failed to load config '%s'", config_setup.config_path);
                    log_fatal("config.init: failed to load '%s'",
                              config_setup.config_path);
                }
                return false;
            }
            log_debug("config.init: loaded from '%s'", config_setup.config_path);
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
    }

    if (!g_config_validate(&g_ctx.config)) {
        zod_set_error("invalid config — see log for details");
        return false;
    }

    g_config_adjust(&g_ctx.config);

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
        const char *title = cvar_get_string(&g_ctx.config.cvars, "window.title",
                                            DEFAULT_CONFIG_WINDOW_TITLE);

        int w = cvar_get_int(&g_ctx.config.cvars, "window.width",
                             DEFAULT_CONFIG_WINDOW_WIDTH);
        int h = cvar_get_int(&g_ctx.config.cvars, "window.height",
                             DEFAULT_CONFIG_WINDOW_HEIGHT);

        uint32_t flags = 0;
        if (cvar_get_bool(&g_ctx.config.cvars, "window.transparent",
                          DEFAULT_CONFIG_WINDOW_TRANSPARENT))
            flags |= SDL_WINDOW_TRANSPARENT;
        g_ctx.window = window_create(title, w, h, flags);
        zod_ngine_apply_config(false);
        zod_console_init();
    }
#endif

    {
        int target_fps = cvar_get_int(&g_ctx.config.cvars, "engine.target_fps",
                                      DEFAULT_CONFIG_TARGET_FPS);
        g_clock_init((uint32_t)target_fps);
        log_debug("clock.init: target fps = %d", target_fps);
    }

    if (dispatch.after_init) dispatch.after_init(user_data);

    log_info("engine.init: ready");
    return true;
}

void zod_ngine_destroy(void) {
    log_debug("engine.destroy: shutting down");
    zod_console_destroy();
    engine_context_destroy();
}

void zod_ngine_apply_config(bool adjust_config) {
    if (adjust_config) g_config_adjust(&g_ctx.config);

    log_set_level(cvar_get_int(&g_ctx.config.cvars, "log.level", LOG_TRACE));

    int target_fps = cvar_get_int(&g_ctx.config.cvars, "engine.target_fps",
                                  DEFAULT_CONFIG_TARGET_FPS);
    g_clock_change_target_fps(target_fps >= 0 ? (uint32_t)target_fps
                                              : DEFAULT_CONFIG_TARGET_FPS);
    window_apply_config(&g_ctx.window);
}

bool zod_should_exit(void) { return g_ctx.should_exit; }
void zod_request_exit(void) { g_ctx.should_exit = true; }

bool zod_tick_hot_reload(void) {
    if (!g_ctx.config.config_file_watcher) return false;
    if (file_watcher_check(g_ctx.config.config_file_watcher) != FILE_CHANGED)
        return false;
    log_info("config.watcher: '%s' changed, reloading",
             g_ctx.config.config_file_watcher->path);
    if (!g_config_reload_from_file(&g_ctx.config)) {
        log_warn("config.watcher: reload failed — keeping previous config");
        return false;
    }
    zod_ngine_apply_config(true);
#if DEBUG
    g_config_print(&g_ctx.config);
#endif
    return true;
}

void zod_begin_drawing(void) { render_begin(); }
void zod_end_drawing(void) { render_end(); }

#endif
