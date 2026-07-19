#ifndef ZOD_NGINE_CONFIG_H
#define ZOD_NGINE_CONFIG_H

#include <ngine.lib/cvar.h>

typedef struct config config;

void config_init(config *cfg);
void config_destroy(config *cfg);

void config_add_user_constraints(config *cfg, const cvar_constraint *entries,
                                   size_t count);

bool config_reload_from_file(config *cfg);

// @info(engine-config): adjust user config to enfore required values
bool config_adjust(config *cfg);

// @info(engine-config): reject fatal invariants (window size, target_fps must be > 0)
bool config_validate(config *cfg);

// debug
void config_print(config *cfg);

#endif
