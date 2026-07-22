#ifndef ZOD_NGINE_CLOCK_H
#define ZOD_NGINE_CLOCK_H

#include <stdint.h>

typedef struct engine_clock engine_clock;

void clock_priv_init(uint32_t target_fps);
void clock_priv_change_target_fps(uint32_t target_fps);
void clock_priv_update(void);
void clock_priv_sleep_to_target_fps(void);

#endif
