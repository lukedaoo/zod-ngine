#ifndef ENGINE_CONTEXT_INTERNAL_H
#define ENGINE_CONTEXT_INTERNAL_H

#include "../../engine_context.h"
#include "../config/config_internal.h"
#include "../clock/clock_internal.h"
#include "../window/window_internal.h"

struct engine_context {
    g_config config;
    g_clock  clock;
    Window   window;
    bool     should_exit;
};

#endif
