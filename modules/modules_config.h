#ifndef MODULES_CONFIG_H
#define MODULES_CONFIG_H

// Capacity overrides for all modules. Included automatically by index.h —
// do not include manually. Uncomment a line to override its default.

// ------- Cvars
// cvar.h — max number of cvars a table can hold.
// @default 1024
// #define CVAR_TABLE_MAX_SIZE 1024

// cvar.h — initial cvar_table.data capacity before first grow. @
// @default 8
// #define CVAR_DEFAULT_CAPACITY 8

// cvar.h — max length of a cvar name, including null terminator
// @default 64
// #define CVAR_NAME_MAX 64

// ------- File Watcher
// file_watcher.h — max watched file path length, including null terminator
// @default 256
// #define FILE_WATCHER_PATH_MAX 256

// ------- INI
// ini.h — max length of one line in an ini file
// @default 1024
// #define INI_LINE_STR_MAX_SIZE 1024

// ini.h — max length of a section name in an ini file
// @default 128
// #define INI_SECTION_STR_MAX_SIZE 128

// ------- SCF
// scf.h — max length of one line in an scf file
// @default 1024
// #define SCF_LINE_STR_MAX_SIZE 1024

// scf.h — max length of a section name in an scf file
// @default 128
// #define SCF_SECTION_STR_MAX_SIZE 128

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

#endif
