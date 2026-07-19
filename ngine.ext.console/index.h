#ifndef NGINE_EXT_CONSOLE_INDEX_H
#define NGINE_EXT_CONSOLE_INDEX_H

#include "ngine.core/index.h"

#include "console_config.h"
#include "console.h"

#ifdef ZOD_NGINE_IMPLEMENTATION
#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#include "internal/console/console_platform_gl.c"
#endif
#include "internal/console/console.c"
#endif

#endif
