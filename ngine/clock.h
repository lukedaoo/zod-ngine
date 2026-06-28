#ifndef ZOD_CLOCK_CONFIG_H
#define ZOD_CLOCK_CONFIG_H

#include <stdint.h>

typedef struct g_clock g_clock;

void g_clock_init(uint32_t target_fps);
void g_clock_update(void);
void g_clock_sleep_to_target_fps(void);

#endif
