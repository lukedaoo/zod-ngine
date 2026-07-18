#ifndef ZOD_NGINE_INDEX_H
#define ZOD_NGINE_INDEX_H

#ifdef ZOD_NGINE_IMPLEMENTATION

#if DEBUG
#define ALL_MODULES_LOG_ENABLED
#endif

#define CARG_IMPLEMENTATION
#define CVAR_IMPLEMENTATION
#define CVAR_LOAD_IMPLEMENTATION
#define INI_IMPLEMENTATION
#define SCF_IMPLEMENTATION

#define LOG_IMPLEMENTATION
#define LOG_USE_SIMPLE

#define FILE_WATCHER_IMPLEMENTATION
#define SIMPLE_FONT_IMPLEMENTATION
#include <modules/index.h>

#endif

#include "version.h"
#include "ngine_config.h"
#include "config.h"
#include "clock.h"
#include "input.h"
#include "engine_context.h"
#include "window.h"
#include "render.h"
#include "common.h"
#include "console.h"

#include "zod_error.h"
#include "zod_ngine.h"
#include "render_text.h"

#ifdef ZOD_NGINE_IMPLEMENTATION

#include "internal/config/config.c"
#include "internal/clock/clock.c"
#include "internal/input/input.c"
#include "internal/engine_context/engine_context.c"
#include "internal/window/window.c"
#include "internal/render/render_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#include "internal/render/render_backend_gl.c"
#elif RENDER_BACKEND == RENDER_BACKEND_VULKAN
#include "internal/render/render_backend_vulkan.c"
#endif
#include "internal/render/render.c"

#include "internal/zod_ngine/zod_ngine.c"
#include "internal/zod_ngine/zod_ngine_config.c"
#include "internal/zod_ngine/zod_ngine_clock.c"
#include "internal/zod_ngine/zod_ngine_input.c"
#include "internal/zod_ngine/zod_ngine_console.c"
#include "internal/render/render_text.c"
#include "internal/error/zod_error.c"
#include "internal/common/common.c"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#include "internal/console/console_platform_gl.c"
#endif
#include "internal/console/console.c"

#endif

#endif
