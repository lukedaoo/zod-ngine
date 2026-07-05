#ifndef ZOD_NGINE_WINDOW_H
#define ZOD_NGINE_WINDOW_H

#include <stdint.h>

typedef struct g_window g_window;

g_window window_create(const char *title, int width, int height, uint32_t flags);
void   window_destroy(g_window *window);

bool window_apply_config(g_window *window);

#endif
