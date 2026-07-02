#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "index.h"

#define CONFIG_PATH "run-tree/data/engine.scf"

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
              zod_config_get_int("window.width", 800),
              zod_config_get_int("window.height", 600),
              zod_config_get_string("window.title", "zod-ngine"),
              zod_config_get_bool("window.vsync", true),
              zod_config_get_int("log.level", 0));
    log_debug("engine.after_init: game.difficulty=%d",
              zod_config_get_int("game.difficulty", 1));
}

int main(const int argc, const char **argv) {
    log_debug("zod-ngine run-tree: starting");
    const zod_engine_dispatch dispatch = {
         .before_init = before_init,
         .load_args   = load_args,
         .after_init  = after_init,
    };

    // Stack-local is fine here: zod_ngine_init copies each entry into the engine's
    // own storage synchronously, before this array ever goes out of scope.
    const cvar_constraint app_constraints[] = {
         {.name     = "game.difficulty",
          .expected = CVAR_INT,
          .range    = {.has_min = true, .min.i = 1, .has_max = true, .max.i = 5}},
    };

    const zod_engine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path       = CONFIG_PATH,
                          .hot_reload        = true,
                          .constraints       = app_constraints,
                          .constraints_count = 1,
                          .load_config_func  = load_config_from_file_default},
         .dispatch     = dispatch,
    };

    if (!zod_ngine_init(params)) return 1;

    uint32_t fps_frames = 0;
    float    fps_accum  = 0.0f;

    while (!zod_should_exit()) {
        zod_clock_update();
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zod_request_exit();
        }
        zod_input_update();
        zod_tick_hot_reload();

        if (zod_key_pressed(SDL_SCANCODE_GRAVE)) zod_console_toggle();

        zod_begin_drawing();
        zod_console_draw();

        fps_frames++;
        fps_accum += zod_clock_delta();
        if (fps_accum >= 1.0f) {
            log_info("engine.fps: %u, %f", fps_frames, (double)zod_clock_dt());
            fps_frames = 0;
            fps_accum -= 1.0f;
        }
        zod_end_drawing();
        zod_clock_sleep_to_target_fps();
    }
    zod_ngine_destroy();
    return 0;
}
