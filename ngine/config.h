#ifndef ZOD_NGINE_CONFIG_H
#define ZOD_NGINE_CONFIG_H

typedef struct g_config g_config;

void g_config_seed_preset(g_config *cfg);

bool g_config_reload_from_file(g_config *cfg);

// @info(engine-config): adjust user config to enfore required values
bool g_config_adjust(g_config *cfg);

// debug
void g_config_print(g_config *cfg);

#endif
