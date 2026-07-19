#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/cvar.h>

#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

int zod_config_get_int(const char *name, int fallback) {
    return cvar_get_int(&g_ctx.config.cvars, name, fallback);
}

float zod_config_get_float(const char *name, float fallback) {
    return cvar_get_float(&g_ctx.config.cvars, name, fallback);
}

bool zod_config_get_bool(const char *name, bool fallback) {
    return cvar_get_bool(&g_ctx.config.cvars, name, fallback);
}

const char *zod_config_get_string(const char *name, const char *fallback) {
    return cvar_get_string(&g_ctx.config.cvars, name, fallback);
}

bool zod_config_set_int(const char *name, int value) {
    return cvar_set_int(&g_ctx.config.cvars, name, value);
}

bool zod_config_set_float(const char *name, float value) {
    return cvar_set_float(&g_ctx.config.cvars, name, value);
}

bool zod_config_set_bool(const char *name, bool value) {
    return cvar_set_bool(&g_ctx.config.cvars, name, value);
}

bool zod_config_set_string(const char *name, const char *value) {
    return cvar_set_string(&g_ctx.config.cvars, name, value);
}

#endif
