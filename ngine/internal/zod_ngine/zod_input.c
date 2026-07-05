#ifdef ZOD_NGINE_IMPLEMENTATIOj

#include "../../zod_ngine.h"
#include "../../input.h"
#include "../engine_context/engine_context_internal.h"

bool zod_input_key_down(zod_key_t key) { return input_key_down(key); }
bool zod_input_key_pressed(zod_key_t key) { return input_key_pressed(key); }
bool zod_input_key_released(zod_key_t key) { return input_key_released(key); }

#endif
