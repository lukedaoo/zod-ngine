#ifndef ZOD_NGINE_WINDOW_H
#define ZOD_NGINE_WINDOW_H

#include <stdint.h>

typedef struct window window;

window window_create(const char *title, int width, int height, uint32_t flags);
void   window_destroy(window *window);

bool window_apply_config(window *window);

#endif
