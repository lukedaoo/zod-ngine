#ifdef ZOD_NGINE_IMPLEMENTATION

#include <string.h>
#include <SDL3/SDL.h>

#include "../engine_context/engine_context_internal.h"

#include "clock_internal.h"

void g_clock_init(uint32_t target_fps) {
    memset(&g_ctx.clock, 0, sizeof(g_ctx.clock));

    g_ctx.clock.frame_rate  = target_fps;
    g_ctx.clock.frame_delay = 1.0f / (float)target_fps;

    g_ctx.clock.time_scale = 1.0f;
    g_ctx.clock.paused     = false;

    g_ctx.clock.last_tick = SDL_GetPerformanceCounter();
}

void g_clock_update(void) {
    uint64_t now_tick = SDL_GetPerformanceCounter();
    uint64_t freq     = SDL_GetPerformanceFrequency();

    double raw_dt         = (double)(now_tick - g_ctx.clock.last_tick) / (double)freq;
    g_ctx.clock.last_tick = now_tick;

    g_ctx.clock.delta = (float)raw_dt;
    g_ctx.clock.dt = g_ctx.clock.paused ? 0.0f : (float)(raw_dt * g_ctx.clock.time_scale);

    g_ctx.clock.elapsed += raw_dt;

    g_ctx.clock.last = g_ctx.clock.now;
    g_ctx.clock.now  = (float)g_ctx.clock.elapsed;

    g_ctx.clock.frame_last = g_ctx.clock.now;

    g_ctx.clock.frame_count++;
}

void g_clock_sleep_to_target_fps(void) {
    float frame_elapsed = g_ctx.clock.now - g_ctx.clock.frame_last;

    if (frame_elapsed < g_ctx.clock.frame_delay) {
        float sleep_time = g_ctx.clock.frame_delay - frame_elapsed;
        SDL_Delay((uint32_t)(sleep_time * 1000.0f));
    }
}

#endif
