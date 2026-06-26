#ifndef G_CONFIG_INTERNAL_H
#define G_CONFIG_INTERNAL_H

#include "../../../modules/cvar.h"
#include "../../g_config.h"

struct g_config {
    cvar_table cvars;
#ifdef G_CONFIG_USER_TYPE
    G_CONFIG_USER_TYPE user;
#endif
};

void            g_config_seed_preset(g_config *cfg);
extern g_config g_config_storage;

#endif
