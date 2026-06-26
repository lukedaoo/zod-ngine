#ifndef ZOD_ENGINE_INTERNAL_H
#define ZOD_ENGINE_INTERNAL_H
#include <stdbool.h>
#include "../../../modules/cvar.h"
#include "../../zod_ngine.h"

struct zod_engine_dispatch {
    void (*before_init)(void);
    bool (*load_config_from_file)(const char *filepath, cvar_table *cvars);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
};

struct zod_engine_init_params {
    int                 argc;
    const char        **argv;
    const char         *config_path;
    zod_engine_dispatch dispatch;
};

#endif
