#ifndef ZOD_NGINE_CONFIG_H
#define ZOD_NGINE_CONFIG_H

#include <modules/cvar.h>
#include <modules/cvar_load.h>

typedef struct g_config g_config;

void g_config_init(g_config *cfg);
void g_config_destroy(g_config *cfg);

void g_config_add_user_constraints(g_config *cfg, const cvar_constraint *entries,
                                   size_t count);

bool g_config_reload_from_file(g_config *cfg);

// @info(engine-config): adjust user config to enfore required values
bool g_config_adjust(g_config *cfg);

// @info(engine-config): reject fatal invariants (window size, target_fps must be > 0)
bool g_config_validate(g_config *cfg);

// debug
void g_config_print(g_config *cfg);

#endif
