#ifndef ZOD_NGINE_INDEX_H
#define ZOD_NGINE_INDEX_H

#include "engine_context.h"
#include "zod_ngine.h"

#ifdef ZOD_NGINE_IMPLEMENTATION

//
// Modules usage: log, carg
//
#define CARG_IMPLEMENTATION

#if DEBUG
#define MODULE_LOG_ENABLED
#endif

#define LOG_IMPLEMENTATION
#define LOG_USE_SIMPLE

#include "../modules/index.h"

#include "internal/engine_context_internal.c"
#include "internal/zod_ngine_internal.c"
#endif

#endif
