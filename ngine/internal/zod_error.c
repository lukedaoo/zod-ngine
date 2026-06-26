#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>

#include "../zod_error.h"

static __thread char zod_error_buffer[1024];

void zod_set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(zod_error_buffer, sizeof(zod_error_buffer), fmt, args);
    va_end(args);
}

const char *zod_get_error(void) { return zod_error_buffer; }

#endif
