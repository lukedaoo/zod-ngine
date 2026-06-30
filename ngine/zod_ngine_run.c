#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "index.h"

#define CONFIG_PATH "run-tree/data/engine.scf"

void before_init(void) {
    log_debug("engine.before_init: called");
    log_set_level(LOG_WARN);
}

static bool load_config_from_file_custom(const char *filepath, cvar_table *cvars) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        log_warn(
             "config.load: '%s' has no file extension — cannot determine format, use "
             ".scf or .ini",
             filepath);
        return false;
    }
    if (strcmp(ext, ".scf") == 0) {
        return cvar_load_scf(cvars, filepath, g_ctx.config.user_schema, false);
    }
    if (strcmp(ext, ".ini") == 0) {
        return cvar_load_ini(cvars, filepath, g_ctx.config.user_schema, false);
    }
    log_warn("config.load: unsupported extension '%s' in '%s' — use .scf or .ini", ext,
             filepath);
    return false;
}

bool load_args(const int argc, const char **argv, cvar_table *cvars) {
    carg_register_t defs[] = {
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

void after_init(void) {
    log_debug("engine.after_init: window=%dx%d title='%s' vsync=%d log_level=%d",
              config_get_int("window.width", 800), config_get_int("window.height", 600),
              config_get_string("window.title", "zod-ngine"),
              config_get_bool("window.vsync", true), config_get_int("log.level", 0));
}

int main(const int argc, const char **argv) {
    const zod_engine_dispatch dispatch = {
         .before_init = before_init,
         .load_args   = load_args,
         .after_init  = after_init,
    };

    const zod_engine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path = CONFIG_PATH,
                          .hot_reload  = true,
                          .schema =
                               &(cvar_schema){
                                    .entries =
                                         (cvar_schema_entry[]){
                                              {"window.width", CVAR_INT},
                                              {"window.height", CVAR_INT},
                                              {"window.vsync", CVAR_BOOL},
                                              {"window.title", CVAR_STRING},
                                         },
                                    .count = 4,
                               },
                          .load_config_func = load_config_from_file_custom},
         .dispatch     = dispatch,
    };

    if (!zod_ngine_init(params)) return 1;

    uint32_t fps_frames = 0;
    float    fps_accum  = 0.0f;

    while (!zod_should_exit()) {
        g_clock_update();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zod_request_exit();
        }

        zod_tick_hot_reload();

        zod_begin_drawing();
        zod_end_drawing();

        fps_frames++;
        fps_accum += clock_delta();
        if (fps_accum >= 1.0f) {
            log_info("engine.fps: %u", fps_frames);
            fps_frames = 0;
            fps_accum -= 1.0f;
        }

        g_clock_sleep_to_target_fps();
    }
    zod_ngine_destroy();
    return 0;
}
