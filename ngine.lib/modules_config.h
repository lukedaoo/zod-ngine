#ifndef MODULES_CONFIG_H
#define MODULES_CONFIG_H

// Capacity overrides for all modules. Included automatically by index.h —
// do not include manually. Uncomment a line to override its default.

// Define ALL_MODULES_LOG_ENABLED before including index.h to turn on log_*
// diagnostics for every module at once. Each module also has its own
// <MODULE>_LOG_ENABLED flag below for turning logging on per-module instead.
#ifdef ALL_MODULES_LOG_ENABLED
#define CVAR_LOG_ENABLED         1
#define CVAR_LOAD_LOG_ENABLED    1
#define INI_LOG_ENABLED          1
#define SCF_LOG_ENABLED          1
#define CARG_LOG_ENABLED         1
#define CARG_TO_CVAR_LOG_ENABLED 1
#define FILE_WATCHER_LOG_ENABLED 1
#define ARRAY_LIST_LOG_ENABLED   1
#define FILE_BUFFER_LOG_ENABLED  1
#define COMMAND_LOG_ENABLED      1
#endif

// ------- Log
// log.h — max number of log hooks that can be registered
// @default 4
// #define LOG_MAX_HOOKS 4

// log.h — max length of a single formatted log message
// @default 1024
// #define LOG_MAX_MESSAGE 1024

// log.h — use the simple (no color/timestamp) log line format
// @default undefined
// #define LOG_USE_SIMPLE

// ------- File Buffer
// file_buffer.h — enable log_* diagnostics for this module.
// @default 0
// #define FILE_BUFFER_LOG_ENABLED 0

// ------- Cvars
// cvar.h — max number of cvars a table can hold.
// @default 1024
// #define CVAR_TABLE_MAX_SIZE 1024

// cvar.h — initial cvar_table.data capacity before first grow. @
// @default 8
// #define CVAR_DEFAULT_CAPACITY 8

// cvar.h — max length of a cvar name, including null terminator
// @default 40
// #define CVAR_NAME_MAX 40

// cvar.h — enable log_* diagnostics for this module.
// @default 0
// #define CVAR_LOG_ENABLED 0

// ------- Cvar Load
// cvar_load.h — enable log_* diagnostics for this module. Schema type/range
// violations always log regardless of this flag — see cvar_load.h.
// @default 0
// #define CVAR_LOAD_LOG_ENABLED 0

// ------- File Watcher
// file_watcher.h — max watched file path length, including null terminator
// @default 256
// #define FILE_WATCHER_PATH_MAX 256

// file_watcher.h — enable log_* diagnostics for this module.
// @default 0
// #define FILE_WATCHER_LOG_ENABLED 0

// ------- INI
// ini.h — max length of one line in an ini file
// @default 1024
// #define INI_LINE_STR_MAX_SIZE 1024

// ini.h — max length of a section name in an ini file
// @default 128
// #define INI_SECTION_STR_MAX_SIZE 128

// ini.h — enable log_* diagnostics for this module.
// @default 0
// #define INI_LOG_ENABLED 0

// ------- SCF
// scf.h — max length of one line in an scf file
// @default 1024
// #define SCF_LINE_STR_MAX_SIZE 1024

// scf.h — max length of a section name in an scf file
// @default 128
// #define SCF_SECTION_STR_MAX_SIZE 128

// scf.h — enable log_* diagnostics for this module.
// @default 0
// #define SCF_LOG_ENABLED 0

// ------- Carg
// carg.h — enable log_* diagnostics for this module.
// @default 0
// #define CARG_LOG_ENABLED 0

// ------- Carg To Cvar
// carg_to_cvar.h — enable log_* diagnostics for this module.
// @default 0
// #define CARG_TO_CVAR_LOG_ENABLED 0

// ------- Simple Font
// simple_font.h — glyph atlas grid dimensions (cols x rows must hold all
// SIMPLE_FONT_GLYPH_COUNT glyphs).
// @default 16
// #define SIMPLE_FONT_ATLAS_COLS 16
// @default 6
// #define SIMPLE_FONT_ATLAS_ROWS 6

// simple_font.h — rasterized pixel size for the TTF glyph atlas
// @default 32
// #define SIMPLE_FONT_TTF_PIXEL_SIZE 32

// simple_font.h — TTF glyph atlas grid dimensions (cols x rows must hold all
// glyphs).
// @default 16
// #define SIMPLE_FONT_TTF_ATLAS_COLS 16
// @default 6
// #define SIMPLE_FONT_TTF_ATLAS_ROWS 6

// ------ Array List
// collections/array_list.h - initial capacity before first grow
// @default 16
// #define ARRAY_LIST_INITIAL_CAPACITY 16

// collections/array_list.h - growth factor
// @default 1.5
// #define ARRAY_LIST_GROWTH_FACTOR 1.5

// collections/array_list.h - max capacity
// @default 2048
// #define ARRAY_LIST_MAX_CAPACITY 2048

// collections/array_list.h — enable log_* diagnostics for this module.
// @default 0
// #define ARRAY_LIST_LOG_ENABLED 0

// ------- Command
// command.h — max length of a command name, including null terminator
// @default 32
// #define COMMAND_MAX_NAME_LEN 32

// command.h — max number of arguments a command invocation may take
// @default 32
// #define COMMAND_MAX_ARGC 32

// command.h — max length of a single argument, including null terminator
// @default 16
// #define COMMAND_MAX_ARG_LEN 16

// command.h — initial system_commands capacity before first grow
// @default 16
// #define COMMAND_TABLE_SYSTEM_COMMAND_INIT_SIZE 16

// command.h — max number of system commands a table can hold
// @default 256
// #define COMMAND_TABLE_SYSTEM_COMMAND_MAX_SIZE 256

// command.h — initial user_defined_commands capacity before first grow
// @default 16
// #define COMMAND_TABLE_USER_COMMAND_INITIAL_CAPACITY 16

// command.h — enable log_* diagnostics for this module.
// @default 0
// #define COMMAND_LOG_ENABLED 0

#endif
