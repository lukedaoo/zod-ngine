#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../clock.h"
#include "../../zod_ngine.h"
#include "../clock/clock_internal.h"
#include "../engine_context/engine_context_internal.h"

float  zod_clock_dt(void) { return g_ctx.clock.dt; }
float  zod_clock_delta(void) { return g_ctx.clock.delta; }
double zod_clock_now(void) { return g_ctx.clock.now; }
float  zod_clock_frame_time(void) { return (float)(g_ctx.clock.now - g_ctx.clock.last); }
uint32_t zod_clock_frame(void) { return g_ctx.clock.frame_count; }
bool     zod_clock_paused(void) { return g_ctx.clock.paused; }

void zod_clock_update(void) { clock_update(); }
void zod_clock_sleep_to_target_fps(void) { clock_sleep_to_target_fps(); }

void zod_clock_set_time_scale(float scale) { g_ctx.clock.time_scale = scale; }
void zod_clock_set_paused(bool paused) { g_ctx.clock.paused = paused; }

#endif
