#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>

#include "../../console.h"
#include "ngine.core/zod_ngine.h"
#include "console_internal.h"

bool zod_console_toggle(void) { return console_toggle(); }

void zod_console_write(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    console_write_v(fmt, args);
    va_end(args);
}

bool zod_console_draw(void) { return console_draw(); }

bool zod_console_visible(void) { return console_visible(); }

void zod_console_handle_event(console_input_event event) { console_handle_event(event); }

#endif
