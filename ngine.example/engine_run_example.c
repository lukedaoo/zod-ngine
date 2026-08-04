#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#define CONFIG_PATH "run-tree/data/engine.scf"

enum : uint8_t { EVENT_CATEGORY_APP = 5 } app_event_category;

// app_event_ids range [100-200]
enum : uint8_t { APP_EVENT_TICK = 100 } app_event_ids;

#if ZOD_CONSOLE_ENABLED
enum : uint8_t { INPUT_CONTEXT_GAME = 0 } input_contexts;
enum : uint8_t { ACTION_TRIGGER_KEY_PRESSED = 0 } action_triggers;

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

    int                app_userdata    = 42;
    const event_handle app_tick_handle = zngine_event_subscribe(
         EVENT_CATEGORY_APP, APP_EVENT_TICK, on_app_tick, &app_userdata, NULL);

#if ZOD_CONSOLE_ENABLED
    action_executor toggle_console_executor = {.execute = toggle_console_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_GRAVE,
                       "toggle_console", &toggle_console_executor);
#endif

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
#if ZOD_CONSOLE_ENABLED
            zconsole_input_handle(&e);
#endif
        }
        zngine_input_update();
        zngine_tick_hot_reload();

#if ZOD_CONSOLE_ENABLED
        if (zngine_input_key_pressed(SDL_SCANCODE_GRAVE))
            zngine_action_execute_by_name(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                                          "toggle_console", NULL);
#endif

        zngine_begin_drawing();
#if ZOD_CONSOLE_ENABLED
        zconsole_draw();
#endif
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
