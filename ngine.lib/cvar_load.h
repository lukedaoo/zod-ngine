#ifndef CVAR_LOAD_H
#define CVAR_LOAD_H

#include "cvar.h"
#include "cvar_parser.h"
#include "ini.h"
#include "log.h"
#include "scf.h"

// force_reload=true: parse into scratch table, swap only on success.
bool cvar_load_ini(cvar_table *table, const char *path, bool force_reload);
bool cvar_load_scf(cvar_table *table, const char *path, bool force_reload);

#ifdef CVAR_LOAD_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool cvar_strict_handler(const char *section, const char *key, const char *value,
                                void *user) {
    cvar_table *table = user;
    return cvar_parse_and_set(section, key, value, table);
}

typedef bool (*parse_func)(const char *filepath, void *handler, void *user);

static bool cvar_load_internal(cvar_table *table, const char *path, bool force_reload,
                               parse_func parser) {
    if (!force_reload) {
        return parser(path, cvar_strict_handler, table);
    }
    cvar_table next = {0};
    cvar_copy_schema(&next, table);
    if (!parser(path, cvar_strict_handler, &next)) {
        cvar_destroy(&next);
        return false;
    }
    cvar_destroy(table);
    *table = next;
    return true;
}

bool cvar_load_ini(cvar_table *table, const char *path, bool force_reload) {
    return cvar_load_internal(table, path, force_reload, (parse_func)ini_parse);
}

bool cvar_load_scf(cvar_table *table, const char *path, bool force_reload) {
    return cvar_load_internal(table, path, force_reload, (parse_func)scf_parse);
}

#endif
#endif
