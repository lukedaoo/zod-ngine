#ifndef ENGINE_CONTEXT_INTERNAL_H
#define ENGINE_CONTEXT_INTERNAL_H

#include "../../engine_context.h"
#include "../../input.h"
#include "../../window.h"

#include "../config/config_internal.h"
#include "../clock/clock_internal.h"
#include "../input/input_internal.h"
#include "../window/window_internal.h"

struct engine_context {
    g_config config;
    g_clock  clock;
    g_input  input;
    window   window;
    bool     should_exit;
};

#endif
