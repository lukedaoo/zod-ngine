#ifndef CONFIG_INTERNAL_H
#define CONFIG_INTERNAL_H

#include "../../../modules/cvar.h"
#include "../../config.h"

struct g_config {
    cvar_table cvars;
};

#endif
