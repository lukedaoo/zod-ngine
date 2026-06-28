#ifndef CLOCK_INTERNAL_H
#define CLOCK_INTERNAL_H

#include <stdint.h>

#include "../../clock.h"

struct g_clock {
    // --- High precision timing ---
    uint64_t last_tick;   // raw tick timestamp
    double   elapsed;     // total unscaled time
    float    dt;          // scaled delta time
    float    time_scale;  // 1.0 = normal, 0 = paused
    bool     paused;

    // --- Frame timing ---
    float now;    // current time (seconds)
    float last;   // previous time
    float delta;  // unscaled delta

    float frame_last;   // last frame start time
    float frame_delay;  // target frame duration (1 / frame_rate)

    uint32_t frame_rate;   // target FPS
    uint32_t frame_count;  // total frames rendered
};

#endif
