#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include "index.h"

#define CONFIG_PATH "run-tree/data/engine.scf"

void before_init(void) {
    log_set_level(LOG_DEBUG);
    log_debug("before_init");
}

bool load_config_from_file(const char *filepath, cvar_table *merged) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        log_debug("config: no file extension in path: %s", filepath);
        return false;
    }
    if (strcmp(ext, ".scf") == 0) {
        return cvar_load_scf(merged, filepath, cvar_default_config_parser_handler, false);
    }
    if (strcmp(ext, ".ini") == 0) {
        return cvar_load_ini(merged, filepath, cvar_default_config_parser_handler, false);
    }
    return false;
}

bool load_args(const int argc, const char **argv, cvar_table *merged) {
    carg_register_t defs[] = {
         {.flag = "--debug-log", .arg_count = 0, .type = CARG_BOOL, .required = false},
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };
    carg_table cargs = {0};
    if (!carg_parse(defs, 2, argc, argv, &cargs)) {
        return false;
    }

    const char *size_names[] = {"window.width", "window.height"};
    carg_entry_to_cvars(carg_get(&cargs, "--size"), size_names, 2, merged);

    if (carg_get_bool(&cargs, "--debug-log", false)) {
        cvar_set_int(merged, "log.level", LOG_DEBUG);
    }

    return true;
}

void after_init(void) {
    log_debug("config — window: %dx%d title: '%s' vsync: %d log_level: %d",
              g_ctx.config->window_width, g_ctx.config->window_height,
              g_ctx.config->title, g_ctx.config->vsync, g_ctx.config->log_level);
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
