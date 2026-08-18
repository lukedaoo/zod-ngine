#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#define CONFIG_PATH           "run-tree/data/engine.scf"
#define KEYBIND_OVERRIDE_PATH "run-tree/data/keybind_override.scf"

enum : uint8_t { EVENT_CATEGORY_APP = 5 } app_event_category;

// app_event_ids range [100-200]
enum : uint8_t { APP_EVENT_TICK = 100 } app_event_ids;

enum : uint8_t { INPUT_CONTEXT_GAME = 0, INPUT_CONTEXT_SELFTEST = 1 } input_contexts;

#if ZOD_CONSOLE_ENABLED
static action_execute_result toggle_console_execute(const action_table *table,
                                                    action_handle self, void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    zconsole_toggle();
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}
#endif

static event_callback_result on_app_tick(event_context *ctx, void *userdata) {
    const uint32_t *frame = (const uint32_t *)ctx->payload;
    log_debug("app.on_tick: frame=%u userdata=%p", *frame, userdata);
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

typedef struct {
    const char *name;
    bool (*run)(void);
} service_check;

static bool check_config(void) {
    if (zngine_config_get_int("window.width", 0) <= 0) return false;
    if (!zngine_config_set_int("game.difficulty", 3)) return false;
    return zngine_config_get_int("game.difficulty", 0) == 3;
}

static bool check_clock(void) {
    zngine_clock_set_paused(true);
    bool paused_ok = zngine_clock_paused() && zngine_clock_dt() == 0.0f;
    zngine_clock_set_paused(false);
    return paused_ok && !zngine_clock_paused();
}

static bool check_window(void) {
    return zngine_window_width() > 0 && zngine_window_height() > 0;
}

static bool check_font(void) { return zngine_font_primary_get() != NULL; }

static int service_check_event_hits;
static int service_check_event_hits_second;

static event_callback_result check_event_callback(event_context *ctx, void *userdata) {
    (void)userdata;
    if (ctx->payload && *(const int *)ctx->payload == 7) service_check_event_hits++;
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

static event_callback_result check_event_callback_second(event_context *ctx,
                                                         void          *userdata) {
    (void)ctx;
    (void)userdata;
    service_check_event_hits_second++;
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

static bool check_event_manager(void) {
    service_check_event_hits        = 0;
    service_check_event_hits_second = 0;

    const event_handle handle = zngine_event_subscribe(
         EVENT_CATEGORY_APP, APP_EVENT_TICK + 1, check_event_callback, NULL, NULL);
    const event_handle handle_second = zngine_event_subscribe(
         EVENT_CATEGORY_APP, APP_EVENT_TICK + 1, check_event_callback_second, NULL, NULL);
    if (handle == EVENT_HANDLE_INVALID || handle_second == EVENT_HANDLE_INVALID)
        return false;

    int payload = 7;
    zngine_event_emit(EVENT_CATEGORY_APP, APP_EVENT_TICK + 1, &payload, sizeof(payload));
    if (service_check_event_hits != 1 || service_check_event_hits_second != 1)
        return false;

    if (!zngine_event_unsubscribe(handle)) return false;
    zngine_event_emit(EVENT_CATEGORY_APP, APP_EVENT_TICK + 1, &payload, sizeof(payload));

    if (!zngine_event_unsubscribe(handle_second)) return false;
    return service_check_event_hits == 1 && service_check_event_hits_second == 2;
}

static command_execute_result check_command_handler(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){.type = COMMAND_RESULT_INT, .value.i = 99};
}

static bool check_cmd_manager(void) {
    if (!zngine_command_register(COMMAND_GROUP_USER_DEFINED, "selftest",
                                 check_command_handler))
        return false;

    command_execute_result result = zngine_user_command_execute("selftest", 0, NULL);
    bool                   ok = result.type == COMMAND_RESULT_INT && result.value.i == 99;

    ok = zngine_command_unregister(COMMAND_GROUP_USER_DEFINED, "selftest") && ok;
    return ok && zngine_user_command_execute("selftest", 0, NULL).type ==
                      COMMAND_RESULT_COMMAND_NOT_FOUND;
}

static int                   service_check_action_hits;
static action_execute_result check_action_execute(const action_table *table,
                                                  action_handle self, void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    service_check_action_hits++;
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static bool check_action_manager(void) {
    service_check_action_hits    = 0;
    action_executor     executor = {.execute = check_action_execute};
    const action_handle handle =
         zngine_action_bind(INPUT_CONTEXT_SELFTEST, ACTION_TRIGGER_KEY_PRESSED,
                            SDL_SCANCODE_F24, "selftest", &executor);
    if (handle == ACTION_HANDLE_INVALID) return false;

    if (zngine_action_resolve_by_name(INPUT_CONTEXT_SELFTEST, ACTION_TRIGGER_KEY_PRESSED,
                                      "selftest") != handle)
        return false;
    if (zngine_action_resolve_by_key(INPUT_CONTEXT_SELFTEST, ACTION_TRIGGER_KEY_PRESSED,
                                     SDL_SCANCODE_F24) != handle)
        return false;

    zngine_action_execute(handle, NULL);
    return service_check_action_hits == 1 &&
           zngine_action_unbind_by_name(INPUT_CONTEXT_SELFTEST,
                                        ACTION_TRIGGER_KEY_PRESSED, "selftest");
}

static int keybind_key_from_name(const char *name) {
    return (int)SDL_GetScancodeFromName(name);
}
static const char *keybind_key_to_name(const int key) {
    return SDL_GetScancodeName((SDL_Scancode)key);
}

static const keybind_context keybind_selftest_contexts[] = {
     {.name = "game", .context = INPUT_CONTEXT_GAME},
};

static const keybind_vocab keybind_selftest_vocab = {
     .contexts      = keybind_selftest_contexts,
     .context_count = 1,
     .key_from_name = keybind_key_from_name,
     .key_to_name   = keybind_key_to_name,
};

static bool check_keybind_manager(void) {
    action_executor base_executor     = {.execute = check_action_execute};
    action_executor override_executor = {.execute = check_action_execute};

    const action_handle base_handle =
         zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                            SDL_SCANCODE_P, "toggle_pause", &base_executor);
    const action_handle override_handle =
         zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                            SDL_SCANCODE_W, "toggle_pause_override", &override_executor);
    if (base_handle == ACTION_HANDLE_INVALID || override_handle == ACTION_HANDLE_INVALID)
        return false;

    if (!zngine_keybind_manager_load(CONFIG_PATH, &keybind_selftest_vocab)) return false;
    if (!zngine_keybind_manager_merge(KEYBIND_OVERRIDE_PATH, &keybind_selftest_vocab))
        return false;

    const action_handle resolved = zngine_keybind_resolve(
         INPUT_CONTEXT_GAME, SDL_SCANCODE_P, ACTION_TRIGGER_KEY_PRESSED);

    bool ok = resolved == override_handle && resolved != base_handle;

    ok = zngine_action_unbind_by_name(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                                      "toggle_pause") &&
         ok;
    ok = zngine_action_unbind_by_name(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                                      "toggle_pause_override") &&
         ok;
    return ok;
}

static bool check_extensions(void) {
#if ZOD_CONSOLE_ENABLED
    zconsole_write("selftest: console reachable");
    bool was_visible = zconsole_visible();
    zconsole_toggle();
    bool toggled = zconsole_visible() != was_visible;
    zconsole_toggle();
    return toggled && zconsole_visible() == was_visible;
#else
    return true;
#endif
}

static const service_check service_checks[] = {
     {"config", check_config},
     {"clock", check_clock},
     {"window", check_window},
     {"font", check_font},
     {"event_manager", check_event_manager},
     {"cmd_manager", check_cmd_manager},
     {"action_manager", check_action_manager},
     {"keybind_manager", check_keybind_manager},
     {"extensions", check_extensions},
};

static bool run_service_checks(void) {
    size_t failed = 0;
    for (size_t i = 0; i < sizeof(service_checks) / sizeof(service_checks[0]); i++) {
        if (service_checks[i].run()) {
            log_info("selftest: %-15s ok", service_checks[i].name);
        } else {
            log_error("selftest: %-15s FAILED", service_checks[i].name);
            failed++;
        }
    }
    log_info("selftest: %zu/%zu services ok",
             sizeof(service_checks) / sizeof(service_checks[0]) - failed,
             sizeof(service_checks) / sizeof(service_checks[0]));
    return failed == 0;
}

void before_init(void *user_data) {
    (void)user_data;
    log_debug("engine.before_init: called");
}

bool load_args(const int argc, const char **argv, cvar_table *cvars) {
    carg_register defs[] = {
         {.flag = "--log-level", .arg_count = 1, .type = CARG_STRING, .required = false},
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };

    carg_table cargs = {0};
    if (!carg_parse(defs, 2, argc, argv, &cargs)) {
        return false;
    }

    const char *size_names[] = {"window.width", "window.height"};
    if (!carg_entry_to_cvars(carg_get(&cargs, "--size"), size_names, 2, cvars)) {
        return false;
    }

    const char *log_level[] = {"log.level"};
    if (!carg_entry_to_cvars(carg_get(&cargs, "--log-level"), log_level, 1, cvars)) {
        return false;
    }

    return true;
}

void after_init(void *user_data) {
    (void)user_data;
    log_debug("engine.after_init: window=%dx%d title='%s' vsync=%d log_level=%d",
              zngine_config_get_int("window.width", 800),
              zngine_config_get_int("window.height", 600),
              zngine_config_get_string("window.title", "zod-ngine"),
              zngine_config_get_bool("window.vsync", true),
              zngine_config_get_int("log.level", 0));
    log_debug("engine.after_init: game.difficulty=%d",
              zngine_config_get_int("game.difficulty", 1));
}

int main(const int argc, const char **argv) {
    log_debug("zod-ngine run-tree: starting");
    const zngine_dispatch dispatch = {
         .before_init = before_init,
         .load_args   = load_args,
         .after_init  = after_init,
    };

    const cvar_constraint app_config_constraints[] = {
         {.name     = "game.difficulty",
          .expected = CVAR_INT,
          .range    = {.has_min = true, .min.i = 1, .has_max = true, .max.i = 5}}};

    const zngine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path       = CONFIG_PATH,
                          .hot_reload        = true,
                          .constraints       = app_config_constraints,
                          .constraints_count = 1,
                          .load_config_func  = load_config_from_file_default},
         .dispatch     = dispatch,
    };

#if ZOD_CONSOLE_ENABLED
    zconsole_ext_install();
#endif

    if (!zngine_init(params)) return 1;
    render_text_init();

    run_service_checks();

    int                app_userdata    = 42;
    const event_handle app_tick_handle = zngine_event_subscribe(
         EVENT_CATEGORY_APP, APP_EVENT_TICK, on_app_tick, &app_userdata, NULL);

#if ZOD_CONSOLE_ENABLED
    action_executor toggle_console_executor = {.execute = toggle_console_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, 0,
                       "toggle_console", &toggle_console_executor);
#endif

    zngine_keybind_manager_load(CONFIG_PATH, &keybind_selftest_vocab);

    uint32_t fps_frames = 0;
    float    fps_accum  = 0.0f;

    while (!zngine_should_exit()) {
        zngine_clock_update();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zngine_request_exit();
            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                sys_event_resize_payload payload = {.width  = e.window.data1,
                                                    .height = e.window.data2};
                event_context            ctx     = {
                     .identifier   = {.category = EVENT_CATEGORY_SYSTEM,
                                      .event_id = SYS_EVENT_ON_RESIZE_WINDOW},
                     .payload      = &payload,
                     .payload_size = sizeof(payload)};
                zngine_event_publish(&ctx);
            }
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                const action_handle handle = zngine_keybind_resolve(
                     INPUT_CONTEXT_GAME, (int)e.key.scancode, ACTION_TRIGGER_KEY_PRESSED);
                if (handle != ACTION_HANDLE_INVALID) zngine_action_execute(handle, NULL);
            }
            zngine_extensions_handle_event(&e);
        }
        zngine_input_update();
        zngine_tick_hot_reload();

        zngine_begin_drawing();
        zngine_extensions_draw();
        render_text_flush();

        fps_frames++;
        fps_accum += zngine_clock_delta();
        if (fps_accum >= 1.0f) {
            log_debug("engine.fps: %u, dt: %f, frame_count: %u", fps_frames,
                      (double)zngine_clock_dt(), zngine_clock_frame());
            fps_frames = 0;
            fps_accum -= 1.0f;
        }
        zngine_end_drawing();
        zngine_clock_sleep_to_target_fps();
    }
    zngine_event_unsubscribe(app_tick_handle);
    render_text_destroy();
    zngine_destroy();
    return 0;
}
