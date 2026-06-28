#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "index.h"

#define CONFIG_PATH "run-tree/data/engine.scf"

void before_init(void) {
    log_set_level(LOG_DEBUG);
    log_debug("before_init");
}

static bool load_config_from_file_custom(const char *filepath, cvar_table *cvars) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        log_debug("config: no file extension in path: %s", filepath);
        return false;
    }
    if (strcmp(ext, ".scf") == 0) {
        return cvar_load_scf(cvars, filepath, cvar_default_config_parser_handler, false);
    }
    if (strcmp(ext, ".ini") == 0) {
        return cvar_load_ini(cvars, filepath, cvar_default_config_parser_handler, false);
    }
    log_debug("config: unsupported file extension: %s", ext);
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
    log_debug("config — window: %d x %d, title: '%s', vsync: %d, log_level: %s",
              config_get_int("window.width", 800), config_get_int("window.height", 600),
              config_get_string("window.title", "zod-ngine"),
              config_get_bool("window.vsync", true),
              config_get_string("log.level", "error"));

    for (size_t i = 0; i < g_ctx.config.cvars.size; ++i) {
        const char *name = g_ctx.config.cvars.data[i].name;
        log_debug("config: %s", name);
    }
}

int main(const int argc, const char **argv) {
    const zod_engine_dispatch dispatch = {
         .before_init = before_init,
         .load_args   = load_args,
         .after_init  = after_init,
    };

    const zod_engine_init_params params = {
         .argc              = argc,
         .argv              = argv,
         .config_file_setup = {.config_path      = CONFIG_PATH,
                               .hot_reload       = true,
                               .load_config_func = load_config_from_file_custom},
         .dispatch          = dispatch,
    };

    zod_ngine_init(params);
    main_loop();
    zod_ngine_destroy();
    return 0;
}
