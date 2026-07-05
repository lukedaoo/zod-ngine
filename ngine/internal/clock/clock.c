#ifdef ZOD_NGINE_IMPLEMENTATION

#include <string.h>
#include <SDL3/SDL.h>

#include <modules/log.h>
#include "../engine_context/engine_context_internal.h"

#include "clock_internal.h"

void clock_init(uint32_t target_fps) {
    if ((int32_t)target_fps < 0) {
        log_error("clock.init: negative target_fps %d rejected — ignoring",
                  (int32_t)target_fps);
        return;
    }

    memset(&g_ctx.clock, 0, sizeof(g_ctx.clock));
    g_ctx.clock.freq        = SDL_GetPerformanceFrequency();
    g_ctx.clock.last_tick   = SDL_GetPerformanceCounter();
    g_ctx.clock.frame_rate  = target_fps;
    g_ctx.clock.frame_delay = target_fps ? 1.0f / (float)target_fps : 0.0f;
    g_ctx.clock.time_scale  = 1.0f;
    g_ctx.clock.paused      = false;
}

void clock_change_target_fps(uint32_t target_fps) {
    if ((int32_t)target_fps < 0) {
        log_error("clock.change_target_fps: negative fps %d rejected — ignoring",
                  (int32_t)target_fps);
        return;
    }

    log_debug("clock.change_target_fps: %u", target_fps);
    g_ctx.clock.frame_rate  = target_fps;
    g_ctx.clock.frame_delay = target_fps ? 1.0f / (float)target_fps : 0.0f;
}

void clock_update(void) {
    uint64_t now_tick = SDL_GetPerformanceCounter();
    double raw_dt = (double)(now_tick - g_ctx.clock.last_tick) / (double)g_ctx.clock.freq;
    g_ctx.clock.last_tick = now_tick;

    if (raw_dt > DEFAULT_CONFIG_CLOCK_STAMP) raw_dt = DEFAULT_CONFIG_CLOCK_STAMP;

    g_ctx.clock.delta = (float)raw_dt;
    g_ctx.clock.dt = g_ctx.clock.paused ? 0.0f : (float)(raw_dt * g_ctx.clock.time_scale);
    g_ctx.clock.elapsed += raw_dt;
    g_ctx.clock.last = g_ctx.clock.now;
    g_ctx.clock.now  = g_ctx.clock.elapsed;
    g_ctx.clock.frame_count++;
}

void clock_sleep_to_target_fps(void) {
    if (g_ctx.clock.frame_delay <= 0.0f) return;
    if (cvar_get_bool(&g_ctx.config.cvars, "window.vsync", true)) return;

    double   freq  = (double)g_ctx.clock.freq;
    uint64_t start = g_ctx.clock.last_tick;

    for (;;) {
        double taken     = (double)(SDL_GetPerformanceCounter() - start) / freq;
        double remaining = g_ctx.clock.frame_delay - taken;
        if (remaining <= 0.0) break;
        if (remaining > 0.002) SDL_DelayNS((uint64_t)((remaining - 0.001) * 1e9));
    }
}

#endif
