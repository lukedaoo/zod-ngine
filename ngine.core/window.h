#ifndef ZOD_NGINE_WINDOW_H
#define ZOD_NGINE_WINDOW_H

#include <stdint.h>

typedef struct window window;

window window_priv_create(const char *title, int width, int height, uint32_t flags);
void   window_priv_destroy(window *window);

bool window_priv_apply_config(window *window);
void window_priv_notify_resized(window *window, int width, int height);

#endif
