#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "index.h"

#define CONFIG_PATH "run-tree/data/engine.scf"

void before_init(void) {
    log_set_level(LOG_DEBUG);
    log_debug("before_init");
}

bool load_config_from_file(const char *filepath, cvar_table *cvars) {
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
    return false;
}

bool load_args(const int argc, const char **argv, cvar_table *cvars) {
    carg_register_t defs[] = {
         {.flag = "--debug-log", .arg_count = 0, .type = CARG_BOOL, .required = false},
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };
    carg_table cargs = {0};
    if (!carg_parse(defs, 2, argc, argv, &cargs)) {
        return false;
    }

    const char *size_names[] = {"window.width", "window.height"};
    carg_entry_to_cvars(carg_get(&cargs, "--size"), size_names, 2, cvars);

    if (carg_get_bool(&cargs, "--debug-log", false)) {
        cvar_set_int(cvars, "log.level", LOG_DEBUG);
    }

    return true;
}

void after_init(void) {
    cvar_table *cvars = &g_ctx.config->cvars;
    log_debug("config — window: %dx%d title: '%s' vsync: %d log_level: %d",
              cvar_get_int(cvars, "window.width", 800),
              cvar_get_int(cvars, "window.height", 600),
              cvar_get_string(cvars, "window.title", "zod-ngine"),
              cvar_get_bool(cvars, "window.vsync", true),
              cvar_get_int(cvars, "log.level", 0));

    for (size_t i = 0; i < g_ctx.config->cvars.size; ++i) {
        const char *name = g_ctx.config->cvars.data[i].name;
        log_debug("config: %s", name);
    }
}

int main(const int argc, const char **argv) {
    const zod_engine_dispatch dispatch = {
         .before_init           = before_init,
         .load_config_from_file = load_config_from_file,
         .load_args             = load_args,
         .after_init            = after_init,
    };

    const zod_engine_init_params params = {
         .argc        = argc,
         .argv        = argv,
         .config_path = CONFIG_PATH,
         .dispatch    = dispatch,
    };

    zod_ngine_init(params);
    zod_ngine_destroy();
    return 0;
}
