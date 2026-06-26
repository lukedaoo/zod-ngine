#ifndef G_CONFIG_TYPES_H
#define G_CONFIG_TYPES_H

#include "../../../modules/cvar.h"
#include "../../g_config.h"

struct g_config {
    cvar_table cvars;
};

#endif
