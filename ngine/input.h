#ifndef ZOD_NGINE_INPUT_H
#define ZOD_NGINE_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t       zod_key_t;
typedef struct input input;

void input_update(void);

// @info(input) key_down — held this frame
bool input_key_down(zod_key_t key);

// @info(input) key_pressed  — transitioned down this frame
bool input_key_pressed(zod_key_t key);

// @info(input) key_released — transitioned up this frame
bool input_key_released(zod_key_t key);

#endif
