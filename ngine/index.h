#ifndef ZOD_NGINE_INDEX_H
#define ZOD_NGINE_INDEX_H

#include "g_engine_context.h"
#include "zod_ngine.h"

#ifdef ZOD_NGINE_IMPLEMENTATION
#if DEBUG
#define MODULE_LOG_ENABLED
#endif

//
// Modules usage: log, carg, cvar, ini, scf, cvar_load
//
#define CARG_IMPLEMENTATION

#define CVAR_IMPLEMENTATION

#define INI_IMPLEMENTATION
#define SCF_IMPLEMENTATION
#define CVAR_LOAD_IMPLEMENTATION

#define LOG_IMPLEMENTATION
#define LOG_USE_SIMPLE

#include "../modules/index.h"

#include "internal/g_config_internal.c"
#include "internal/g_engine_context_internal.c"
#include "internal/zod_ngine_internal.c"
#endif

#endif
