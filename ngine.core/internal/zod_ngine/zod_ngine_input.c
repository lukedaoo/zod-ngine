#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../zod_ngine.h"
#include "../../input.h"
#include "../engine_context/engine_context_internal.h"

void zngine_input_update(void) { input_priv_update(); }
bool zngine_input_key_down(zod_key_t key) { return input_priv_key_down(key); }
bool zngine_input_key_pressed(zod_key_t key) { return input_priv_key_pressed(key); }
bool zngine_input_key_released(zod_key_t key) { return input_priv_key_released(key); }

#endif
