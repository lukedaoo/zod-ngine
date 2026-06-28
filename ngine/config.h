#ifndef ZOD_NGINE_CONFIG_H
#define ZOD_NGINE_CONFIG_H

typedef struct g_config g_config;

void g_config_seed_preset(g_config *cfg);

bool g_reload_config_from_file(g_config *cfg);

extern g_config g_config_storage;

#endif
