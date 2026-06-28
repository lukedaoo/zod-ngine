#ifndef CONFIG_INTERNAL_H
#define CONFIG_INTERNAL_H

#include "../../../modules/cvar.h"
#include "../../../modules/file_watcher.h"
#include "../../config.h"

struct g_config {
    cvar_table    cvars;
    file_watcher *config_file_watcher;
    bool (*reload_config_func)(const char *filepath, cvar_table *cvars);
};

#endif
