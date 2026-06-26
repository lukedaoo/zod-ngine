#ifdef ZOD_NGINE_IMPLEMENTATION
#include <assert.h>
#include <stdio.h>
#include "../engine_context.h"
#include "../zod_ngine.h"

bool zod_ngine_init(void) {
    g_core.config = NULL;
    printf("zod_ngine_init\n");
    return true;
}

void zod_ngine_destroy(void) { return; }

#endif
