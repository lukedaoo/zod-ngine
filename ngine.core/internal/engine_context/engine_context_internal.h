#ifndef ENGINE_CONTEXT_INTERNAL_H
#define ENGINE_CONTEXT_INTERNAL_H

#include "../../engine_context.h"
#include "../../input.h"
#include "../../window.h"

#include "../config/config_internal.h"
#include "../clock/clock_internal.h"
#include "../input/input_internal.h"
#include "../window/window_internal.h"
#include "ngine.lib/simple_font.h"

struct engine_context {
    config       config;
    engine_clock clock;
    input        input;
    window       window;
    simple_font  primary_font;

    bool should_exit;
};

#endif
