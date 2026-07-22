#ifndef ZOD_NGINE_CONFIG_H
#define ZOD_NGINE_CONFIG_H

#include <ngine.lib/cvar.h>

typedef struct config config;

void config_priv_init(config *cfg);
void config_priv_destroy(config *cfg);

void config_priv_add_user_constraints(config *cfg, const cvar_constraint *entries,
                                   size_t count);

bool config_priv_reload_from_file(config *cfg);

// @info(engine-config): adjust user config to enfore required values
bool config_priv_adjust(config *cfg);

// @info(engine-config): reject fatal invariants (window size, target_fps must be > 0)
bool config_priv_validate(config *cfg);

// debug
void config_priv_print(config *cfg);

#endif
