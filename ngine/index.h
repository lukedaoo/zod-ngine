#ifndef ZOD_NGINE_INDEX_H
#define ZOD_NGINE_INDEX_H

#ifdef ZOD_NGINE_IMPLEMENTATION

#if DEBUG
#define MODULE_LOG_ENABLED
#endif

#define CARG_IMPLEMENTATION
#define CVAR_IMPLEMENTATION
#define CVAR_LOAD_IMPLEMENTATION
#define INI_IMPLEMENTATION
#define SCF_IMPLEMENTATION
#define LOG_IMPLEMENTATION
#define LOG_USE_SIMPLE
#define FILE_WATCHER_IMPLEMENTATION
#include "../modules/index.h"

#endif

#include "config.h"
#include "engine_context.h"
#include "zod_ngine.h"

#ifdef ZOD_NGINE_IMPLEMENTATION

#include "internal/config/config.c"
#include "internal/engine_context/engine_context.c"
#include "internal/zod_ngine/zod_ngine.c"
#include "internal/error/zod_error.c"

#endif

#endif
