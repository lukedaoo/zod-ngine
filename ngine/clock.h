#ifndef ZOD_NGINE_CLOCK_H
#define ZOD_NGINE_CLOCK_H

#include <stdint.h>

typedef struct engine_clock engine_clock;

void clock_init(uint32_t target_fps);
void clock_change_target_fps(uint32_t target_fps);
void clock_update(void);
void clock_sleep_to_target_fps(void);

#endif
