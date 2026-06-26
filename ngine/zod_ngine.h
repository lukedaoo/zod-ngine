#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>

typedef struct zod_engine_dispatch    zod_engine_dispatch;
typedef struct zod_engine_init_params zod_engine_init_params;

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
