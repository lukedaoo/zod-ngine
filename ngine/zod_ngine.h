#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>
#include "../modules/cvar.h"

typedef struct {
    void (*before_init)(void);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
} zod_engine_dispatch;

typedef struct {
    const char *config_path;
    bool        hot_reload;
    bool (*load_config_func)(const char *filepath, cvar_table *cvars);
} zod_config_file_setup_t;

typedef struct {
    int          argc;
    const char **argv;

    zod_config_file_setup_t config_file_setup;

    zod_engine_dispatch dispatch;
} zod_engine_init_params;

//
// Engine lifecycle
//
bool zod_ngine_init(const zod_engine_init_params params);
void zod_ngine_destroy(void);

void main_loop(void);

//
// Config accessors
//
int         config_get_int(const char *name, int fallback);
float       config_get_float(const char *name, float fallback);
bool        config_get_bool(const char *name, bool fallback);
const char *config_get_string(const char *name, const char *fallback);
bool        config_set_int(const char *name, int value);
bool        config_set_float(const char *name, float value);
bool        config_set_bool(const char *name, bool value);
bool        config_set_string(const char *name, const char *value);

#endif
