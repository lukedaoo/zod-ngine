#ifndef CONFIG_INTERNAL_H
#define CONFIG_INTERNAL_H

#include <ngine.lib/cvar.h>
#include <ngine.lib/file_watcher.h>

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

#ifndef DEFAULT_CONFIG_WINDOW_FULLSCREEN
#define DEFAULT_CONFIG_WINDOW_FULLSCREEN false
#endif

#ifndef DEFAULT_CONFIG_WINDOW_RESIZABLE
#define DEFAULT_CONFIG_WINDOW_RESIZABLE false
#endif

// Compile-time resolution lock. Ship a game restricted to fixed resolutions by
// defining ZNGINE_RESOLUTION_LIST as an X-macro at build time, e.g.:
//   -D'ZNGINE_RESOLUTION_LIST(X)=X(1280, 720) X(1600, 900) X(1920, 1080)'
// When defined: the requested window.width/height is snapped to the nearest
// listed preset at creation, and the window is forced non-resizable regardless
// of the window.resizable cvar. When undefined: any size is allowed (dev).

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

void config_priv_seed_preset(config *cfg);

#endif
