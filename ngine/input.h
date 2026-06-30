#ifndef ZOD_NGINE_INPUT_H
#define ZOD_NGINE_INPUT_H

#include <SDL3/SDL_scancode.h>
#include <stdbool.h>

typedef SDL_Scancode   zod_key_t;
typedef struct g_input g_input;

// Call once per frame after SDL_PollEvent loop.
void zod_input_update(void);

// key_down     — held this frame
// key_pressed  — transitioned down this frame
// key_released — transitioned up this frame
bool zod_key_down(zod_key_t key);
bool zod_key_pressed(zod_key_t key);
bool zod_key_released(zod_key_t key);

#endif
