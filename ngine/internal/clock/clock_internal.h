#ifndef CLOCK_INTERNAL_H
#define CLOCK_INTERNAL_H

#include <stdint.h>

#include "../../clock.h"

#ifndef DEFAULT_CONFIG_CLOCK_STAMP
#define DEFAULT_CONFIG_CLOCK_STAMP 0.25F
#endif

struct engine_clock {
    // --- High precision timing ---
    uint64_t freq;        // perf-counter ticks/sec (cached at init)
    uint64_t last_tick;   // tick timestamp at start of current frame
    double   elapsed;     // total sim time (clamped; not wall-clock after a hitch)
    float    dt;          // scaled, pausable delta
    float    time_scale;  // 1.0 = normal
    bool     paused;
    // --- Frame timing ---
    double   now;          // current time (s) = elapsed; double for long sessions
    double   last;         // previous frame's now
    float    delta;        // unscaled delta (raw frame dt, clamped)
    float    frame_delay;  // target frame duration in s (0 = uncapped)
    uint32_t frame_rate;   // target FPS (0 = uncapped)
    uint32_t frame_count;  // total frames
};

#endif
