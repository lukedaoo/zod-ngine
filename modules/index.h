#ifndef INDEX_H
#define INDEX_H

// Convenience umbrella — include all modules in one shot.
// Define all _IMPLEMENTATION macros BEFORE including this file.
// modules_config.h is pulled in automatically and must not be included again.

#include "modules_config.h"

// utils
#include "log.h"
#include "string_view.h"
#include "file_buffer.h"
#include "types.h"

// config
#include "ini.h"
#include "scf.h"
#include "cvar.h"
#include "cvar_load.h"
#include "carg.h"
#include "carg_to_cvar.h"

// io
#include "file_watcher.h"

// graphics
#include "simple_font.h"

// collections
#include "collections/array_list.h"

#endif
