#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../clock.h"
#include "../../zod_ngine.h"
#include "../clock/clock_internal.h"
#include "../engine_context/engine_context_internal.h"

float  zngine_clock_dt(void) { return g_ctx.clock.dt; }
float  zngine_clock_delta(void) { return g_ctx.clock.delta; }
double zngine_clock_now(void) { return g_ctx.clock.now; }
float  zngine_clock_frame_time(void) { return (float)(g_ctx.clock.now - g_ctx.clock.last); }
uint32_t zngine_clock_frame(void) { return g_ctx.clock.frame_count; }
bool     zngine_clock_paused(void) { return g_ctx.clock.paused; }

void zngine_clock_update(void) { clock_priv_update(); }
void zngine_clock_sleep_to_target_fps(void) { clock_priv_sleep_to_target_fps(); }

void zngine_clock_set_time_scale(float scale) { g_ctx.clock.time_scale = scale; }
void zngine_clock_set_paused(bool paused) { g_ctx.clock.paused = paused; }

#endif
