#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>

#define CONFIG_PATH "run-tree/data/minimal_engine_config.scf"

bool load_config_func(const char *filepath, cvar_table *cvars) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) {
        log_warn(
             "config.load: '%s' has no file extension — cannot determine format, use "
             ".scf or .ini",
             filepath);
        return false;
    }

    bool ok = false;
    if (strcmp(ext, ".scf") == 0) {
        ok = cvar_load_scf(cvars, filepath, false);
    } else {
        log_warn("config.load: unsupported extension '%s' in '%s'. Only support scf", ext,
                 filepath);
    }
    return ok;
}

int main(int argc, const char **argv) {
    log_debug("zod-ngine run-tree: starting");
    const zngine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path      = CONFIG_PATH,
                          .hot_reload       = true,
                          .load_config_func = load_config_func},
    };

    if (!zngine_init(params)) return 1;

    while (!zngine_should_exit()) {
        zngine_clock_update();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zngine_request_exit();
        }
        zngine_begin_drawing();
        zngine_end_drawing();
    }

    zngine_destroy();
    return 0;
}
