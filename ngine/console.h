#ifndef ZOD_NGINE_CONSOLE_H
#define ZOD_NGINE_CONSOLE_H

#include <stdbool.h>

void zod_console_init(void);
void zod_console_destroy(void);
void zod_console_toggle(void);
bool zod_console_is_open(void);
void zod_console_draw(void);

#endif
