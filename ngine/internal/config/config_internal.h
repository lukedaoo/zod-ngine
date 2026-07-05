#ifndef CONFIG_INTERNAL_H
#define CONFIG_INTERNAL_H

#include <modules/cvar.h>
#include <modules/cvar_load.h>
#include <modules/file_watcher.h>

#include "../../config.h"

#ifndef DEFAULT_CONFIG_TARGET_FPS
#define DEFAULT_CONFIG_TARGET_FPS 60
#endif

#ifndef DEFAULT_CONFIG_WINDOW_WIDTH
#define DEFAULT_CONFIG_WINDOW_WIDTH 800
#endif

#ifndef DEFAULT_CONFIG_WINDOW_HEIGHT
#define DEFAULT_CONFIG_WINDOW_HEIGHT 600
#endif

#ifndef DEFAULT_CONFIG_WINDOW_TITLE
#define DEFAULT_CONFIG_WINDOW_TITLE "zod-ngine"
#endif

#ifndef DEFAULT_CONFIG_WINDOW_VSYNC
#define DEFAULT_CONFIG_WINDOW_VSYNC true
#endif

#ifndef DEFAULT_CONFIG_WINDOW_CLEAR_COLOR
#define DEFAULT_CONFIG_WINDOW_CLEAR_COLOR 0x141A1AFF
#endif

#ifndef DEFAULT_CONFIG_WINDOW_TRANSPARENT
#define DEFAULT_CONFIG_WINDOW_TRANSPARENT false
#endif

#ifndef DEFAULT_CONFIG_LOG_LEVEL
// LOG_TRACE = 0,
// LOG_DEBUG = 1,
// LOG_INFO = 2,
// LOG_WARN = 3,
// LOG_ERROR = 4,
// LOG_FATAL = 5
#define DEFAULT_CONFIG_LOG_LEVEL 0
#endif

struct config {
    cvar_table    cvars;
    file_watcher *config_file_watcher;
    bool (*reload_config_func)(const char *filepath, cvar_table *cvars);
};

void config_seed_preset(config *cfg);

#endif
