#ifdef ZOD_NGINE_IMPLEMENTATION

#include <SDL3/SDL.h>

#include <ngine.lib/cvar.h>
#include <ngine.lib/log.h>
#include <ngine.lib/file_watcher.h>

#include "../../config.h"
#include "../../cmd_manager.h"
#include "../../event_manager.h"
#include "../../action_manager.h"
#include "../../render.h"
#include "../../render_text.h"
#include "../../zod_error.h"
#include "../../zod_ngine.h"

#include "../config/config_internal.h"
#include "../engine_context/engine_context_internal.h"
#include "zod_ngine_internal.h"

#ifndef ZOD_MAX_EXTENSIONS
#define ZOD_MAX_EXTENSIONS 2
#endif

static zngine_extension g_extensions[ZOD_MAX_EXTENSIONS];
static size_t           g_extensions_count = 0;

void zngine_register_extension(zngine_extension ext) {
    if (g_extensions_count >= ZOD_MAX_EXTENSIONS) {
        log_error("engine.register_extension: max %d extensions already registered",
                  ZOD_MAX_EXTENSIONS);
        return;
    }
    g_extensions[g_extensions_count++] = ext;
}

void extensions_priv_init_config(cvar_table *cvars) {
    for (size_t i = 0; i < g_extensions_count; ++i)
        if (g_extensions[i].init_config) g_extensions[i].init_config(cvars);
}

void zngine_extensions_handle_event(const SDL_Event *event) {
    for (size_t i = 0; i < g_extensions_count; ++i)
        if (g_extensions[i].handle_event) g_extensions[i].handle_event(event);
}

void zngine_extensions_draw(void) {
    for (size_t i = 0; i < g_extensions_count; ++i)
        if (g_extensions[i].draw) g_extensions[i].draw();
}

static void load_font() {
    cvar       *primary_font_cvar = cvar_get(&g_ctx.config.cvars, "asset.font.primary");
    const char *font_path = primary_font_cvar ? primary_font_cvar->value.str.data : NULL;
    log_debug("engine.init: loading font '%s'",
              font_path ? font_path : "(none, using built-in ascii font)");
    g_ctx.primary_font = simple_font_load(font_path);
    render_text_invalidate();
}

#ifdef ZNGINE_RESOLUTION_LIST
// Snap requested window size to the nearest compile-time preset (by squared
// distance). See ZNGINE_RESOLUTION_LIST in config_internal.h.
static void zngine_snap_resolution(int *w, int *h) {
    static const struct {
        int w, h;
    } presets[] = {
#define ZR_ENTRY(W, H) {(W), (H)},
         ZNGINE_RESOLUTION_LIST(ZR_ENTRY)
#undef ZR_ENTRY
    };
    int  best   = 0;
    long best_d = -1;
    for (size_t i = 0; i < sizeof(presets) / sizeof(presets[0]); i++) {
        long dw = (long)*w - presets[i].w;
        long dh = (long)*h - presets[i].h;
        long d  = dw * dw + dh * dh;
        if (best_d < 0 || d < best_d) {
            best_d = d;
            best   = (int)i;
        }
    }
    if (presets[best].w != *w || presets[best].h != *h) {
        log_info("engine.init: resolution %dx%d unsupported, using %dx%d", *w, *h,
                 presets[best].w, presets[best].h);
        *w = presets[best].w;
        *h = presets[best].h;
    }
}
#endif

typedef struct {
    const char *name;
    bool (*init)(const zngine_init_params *params);
    void (*destroy)(void);
} zngine_service;

static bool service_event_manager_init(const zngine_init_params *params) {
    (void)params;
    event_manager_priv_init(&g_ctx.event_manager);
    return true;
}

static void service_event_manager_destroy(void) {
    event_manager_priv_destroy(&g_ctx.event_manager);
}

static bool service_cmd_manager_init(const zngine_init_params *params) {
    (void)params;
    cmd_manager_priv_init(&g_ctx.cmd_manager);
    return true;
}

static void service_cmd_manager_destroy(void) {
    cmd_manager_priv_destroy(&g_ctx.cmd_manager);
}

static bool service_action_manager_init(const zngine_init_params *params) {
    (void)params;
    action_manager_priv_init(&g_ctx.action_manager);
    return true;
}

static void service_action_manager_destroy(void) {
    action_manager_priv_destroy(&g_ctx.action_manager);
}

static bool service_config_init(const zngine_init_params *params) {
    const zngine_config_setup config_setup = params->config_setup;
    const zngine_dispatch     dispatch     = params->dispatch;

    config_priv_init(&g_ctx.config);
    extensions_priv_init_config(&g_ctx.config.cvars);
    config_priv_add_user_constraints(&g_ctx.config, config_setup.constraints,
                                     config_setup.constraints_count);

    if (config_setup.load_config_func && config_setup.config_path) {
        if (!config_setup.load_config_func(config_setup.config_path,
                                           &g_ctx.config.cvars)) {
            zngine_set_error("failed to load config '%s'", config_setup.config_path);
            log_fatal("config.init: failed to load '%s'", config_setup.config_path);
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
        if (!dispatch.load_args(params->argc, params->argv, &g_ctx.config.cvars)) {
            log_warn(
                 "config.init: CLI args parse failed — command-line overrides "
                 "ignored");
        } else {
            log_debug("config.init: CLI args applied");
        }
    }

    if (!config_priv_validate(&g_ctx.config)) {
        zngine_set_error("invalid config — see log for details");
        return false;
    }

    config_priv_adjust(&g_ctx.config);
#ifdef DEBUG
    config_priv_print(&g_ctx.config);
#endif
    return true;
}

static void service_config_destroy(void) { config_priv_destroy(&g_ctx.config); }

#ifndef NGINE_UNIT_TEST
static bool service_platform_init(const zngine_init_params *params) {
    (void)params;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        zngine_set_error("SDL_Init failed: %s", SDL_GetError());
        log_fatal("engine.init: SDL_Init failed — %s, check SDL installation",
                  SDL_GetError());
        return false;
    }
    return true;
}

static void service_platform_destroy(void) { SDL_Quit(); }

static bool service_window_init(const zngine_init_params *params) {
    (void)params;
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

#ifdef ZNGINE_RESOLUTION_LIST
    // Compile-time locked: snap to nearest preset, force non-resizable.
    zngine_snap_resolution(&w, &h);
#else
    if (cvar_get_bool(&g_ctx.config.cvars, "window.resizable",
                      DEFAULT_CONFIG_WINDOW_RESIZABLE))
        flags |= SDL_WINDOW_RESIZABLE;
#endif
    g_ctx.window = window_priv_create(title, w, h, flags);
    zngine_apply_config(false);
    return true;
}

static void service_window_destroy(void) { window_priv_destroy(&g_ctx.window); }
#endif

static bool service_clock_init(const zngine_init_params *params) {
    (void)params;
    int target_fps = cvar_get_int(&g_ctx.config.cvars, "engine.target_fps",
                                  DEFAULT_CONFIG_TARGET_FPS);
    clock_priv_init((uint32_t)target_fps);
    log_debug("clock.init: target fps = %d", target_fps);
    return true;
}

static bool service_extensions_init(const zngine_init_params *params) {
    (void)params;
    for (size_t i = 0; i < g_extensions_count; ++i) {
        if (!g_extensions[i].init) continue;
        if (!g_extensions[i].init()) {
            zngine_set_error("extension %zu failed to initialise", i);
            log_fatal("engine.init: extension %zu failed to initialise", i);
            return false;
        }
    }
    return true;
}

static void service_extensions_destroy(void) {
    for (size_t i = g_extensions_count; i-- > 0;)
        if (g_extensions[i].shutdown) g_extensions[i].shutdown();
}

static const zngine_service g_services[] = {
     {"event_manager", service_event_manager_init, service_event_manager_destroy},
     {"cmd_manager", service_cmd_manager_init, service_cmd_manager_destroy},
     {"action_manager", service_action_manager_init, service_action_manager_destroy},
     {"config", service_config_init, service_config_destroy},
#ifndef NGINE_UNIT_TEST
     {"platform", service_platform_init, service_platform_destroy},
     {"window", service_window_init, service_window_destroy},
#endif
     {"clock", service_clock_init, NULL},
     {"extensions", service_extensions_init, service_extensions_destroy},
};

#define ZNGINE_SERVICE_COUNT (sizeof(g_services) / sizeof(g_services[0]))

static size_t g_services_started = 0;

static void zngine_services_shutdown(void) {
    while (g_services_started > 0) {
        const zngine_service *service = &g_services[--g_services_started];
        if (service->destroy) service->destroy();
        log_debug("%s.destroy: done", service->name);
    }
}

bool zngine_init(const zngine_init_params params) {
    const zngine_dispatch dispatch  = params.dispatch;
    void                 *user_data = params.user_data;

    if (dispatch.before_init) dispatch.before_init(user_data);

    log_info("\nengine.init: starting");

    for (size_t i = 0; i < ZNGINE_SERVICE_COUNT; ++i) {
        const zngine_service *service = &g_services[i];
        g_services_started            = i + 1;
        if (!service->init(&params)) {
            log_fatal("engine.init: service '%s' failed", service->name);
            zngine_services_shutdown();
            return false;
        }
        log_debug("%s.init: ready", service->name);
    }

    if (dispatch.after_init) dispatch.after_init(user_data);

    log_info("engine.init: ready");
    return true;
}

void zngine_destroy(void) {
    log_debug("engine.destroy: shutting down");
    zngine_services_shutdown();
}

void zngine_apply_config(bool adjust_config) {
    if (adjust_config) config_priv_adjust(&g_ctx.config);

    log_set_level(cvar_get_int(&g_ctx.config.cvars, "log.level", LOG_TRACE));

    int target_fps = cvar_get_int(&g_ctx.config.cvars, "engine.target_fps",
                                  DEFAULT_CONFIG_TARGET_FPS);
    clock_priv_change_target_fps(target_fps >= 0 ? (uint32_t)target_fps
                                                 : DEFAULT_CONFIG_TARGET_FPS);
    window_priv_apply_config(&g_ctx.window);

    load_font();

    for (size_t i = 0; i < g_extensions_count; ++i)
        if (g_extensions[i].apply_config) g_extensions[i].apply_config();
}

bool zngine_should_exit(void) { return g_ctx.should_exit; }
void zngine_request_exit(void) { g_ctx.should_exit = true; }

bool zngine_tick_hot_reload(void) {
    if (!g_ctx.config.config_file_watcher) return false;
    if (file_watcher_check(g_ctx.config.config_file_watcher) != FILE_CHANGED)
        return false;
    log_info("config.watcher: '%s' changed, reloading",
             g_ctx.config.config_file_watcher->path);
    if (!config_priv_reload_from_file(&g_ctx.config)) {
        log_warn("config.watcher: reload failed — keeping previous config");
        return false;
    }
    zngine_apply_config(true);

    event_context ctx = {.identifier   = {.category = EVENT_CATEGORY_SYSTEM,
                                          .event_id = SYS_EVENT_ON_CONFIG_RELOAD_FULL},
                         .payload      = NULL,
                         .payload_size = 0};
    event_manager_priv_publish(&g_ctx.event_manager, &ctx);
#if DEBUG
    config_priv_print(&g_ctx.config);
#endif
    return true;
}

void zngine_begin_drawing(void) { render_priv_begin(); }
void zngine_end_drawing(void) { render_priv_end(); }

const simple_font *zngine_font_primary_get(void) { return &g_ctx.primary_font; }

command_handle zngine_command_register(command_group group, const char *name,
                                       command_execute_result (*handler)(int    argc,
                                                                         char **argv)) {
    return cmd_manager_priv_register(&g_ctx.cmd_manager, group, name, handler);
}

bool zngine_command_unregister(command_group group, const char *name) {
    return cmd_manager_priv_unregister(&g_ctx.cmd_manager, group, name);
}

command_execute_result zngine_sys_command_execute(const char *name, int argc,
                                                  char **argv) {
    return cmd_manager_priv_execute(&g_ctx.cmd_manager, COMMAND_GROUP_SYSTEM, name, argc,
                                    argv);
}

command_execute_result zngine_user_command_execute(const char *name, int argc,
                                                   char **argv) {
    return cmd_manager_priv_execute(&g_ctx.cmd_manager, COMMAND_GROUP_USER_DEFINED, name,
                                    argc, argv);
}

event_handle zngine_event_subscribe(const event_category category, const int event_id,
                                    const event_callback callback, void *userdata,
                                    const event_userdata_destroy destroy_fn) {
    return event_manager_priv_subscribe(&g_ctx.event_manager, category, event_id,
                                        callback, userdata, destroy_fn);
}

bool zngine_event_unsubscribe(const event_handle handle) {
    return event_manager_priv_unsubscribe(&g_ctx.event_manager, handle);
}

bool zngine_event_unsubscribe_by_event_identifier(const event_category category,
                                                  const int            event_id) {
    return event_manager_priv_unsubscribe_by_event_identifier(&g_ctx.event_manager,
                                                              category, event_id);
}

void zngine_event_publish(event_context *ctx) {
    event_manager_priv_publish(&g_ctx.event_manager, ctx);
}
#endif
