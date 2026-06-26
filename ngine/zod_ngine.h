#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>
#include "../modules/cvar.h"

typedef struct {
    void (*before_init)(void);
    bool (*load_config_from_file)(const char *filepath, cvar_table *cvars);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
} zod_engine_dispatch;

typedef struct {
    int                 argc;
    const char        **argv;
    const char         *config_path;
    zod_engine_dispatch dispatch;
} zod_engine_init_params;

bool zod_ngine_init(const zod_engine_init_params params);
void zod_ngine_destroy(void);

//
// Config accessors
//
int         get_int_config(const char *name, int fallback);
float       get_float_config(const char *name, float fallback);
bool        get_bool_config(const char *name, bool fallback);
const char *get_string_config(const char *name, const char *fallback);

#endif
