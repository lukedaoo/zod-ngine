#ifndef NGINE_EXT_CONSOLE_INDEX_H
#define NGINE_EXT_CONSOLE_INDEX_H

// Set before pulling in ngine.core so its impl block (zod_ngine.c) can tell,
// at the point it's textually processed, whether this umbrella — not just
// bare ngine.core/index.h — is the one assembling the final binary. That's
// the only way to know console_apply_config() will actually be linked.
#define NGINE_EXT_CONSOLE_PRESENT

#include "ngine.core/index.h"

#include "console.h"

#ifdef ZOD_NGINE_IMPLEMENTATION
#include "internal/console/zod_ngine_console.c"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#include "internal/console/console_platform_gl.c"
#endif
#include "internal/console/console.c"
#endif

#endif
