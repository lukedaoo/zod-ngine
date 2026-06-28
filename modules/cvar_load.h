#ifndef CVAR_LOAD_H
#define CVAR_LOAD_H

#include "cvar.h"
#include "ini.h"
#include "scf.h"

// @fix:
// password      12312321321 ---> casted as int, not string
// solution: 
bool cvar_load_ini(cvar_table *table, const char *ini_path, ini_handler handler,
                   bool force_reload);

bool cvar_load_scf(cvar_table *table, const char *scf_path, scf_handler handler,
                   bool force_reload);

// Generic cvar_handler: maps any "[section] key = value" to a cvar named
// "section.key", inferring CVAR_BOOL/CVAR_INT/CVAR_FLOAT/CVAR_STRING from
// the value text.
bool cvar_default_config_parser_handler(const char *section, const char *key,
                                        const char *value, void *user);
#ifdef CVAR_LOAD_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool cvar_default_config_parser_handler(const char *section, const char *key,
                                        const char *value, void *user) {
    cvar_table *cvars = user;

    char name[CVAR_NAME_MAX];
    snprintf(name, sizeof(name), "%s.%s", section, key);

    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        bool b = strcmp(value, "true") == 0;
        return cvar_set_bool(cvars, name, b);
    }

    char *end;
    long  ival = strtol(value, &end, 10);
    if (*value != '\0' && *end == '\0') {
        int i = (int)ival;
        return cvar_set_int(cvars, name, i);
    }

    float fval = strtof(value, &end);
    if (end != value) {
        if (*end == '\0') {
            return cvar_set_float(cvars, name, fval);
        }
        if ((*end == 'f' || *end == 'F') && end[1] == '\0') {
            return cvar_set_float(cvars, name, fval);
        }
    }

    return cvar_set_string(cvars, name, value);
}

typedef bool (*parse_func)(const char *filepath, void *handler, void *user);

static bool cvar_load_internal(cvar_table *table, const char *filename, void *handler,
                               bool force_reload, parse_func parser) {
    if (!force_reload) {
        return parser(filename, handler, table);
    }

    cvar_table next = {0};
    if (!parser(filename, handler, &next)) {
        cvar_destroy(&next);
        return false;
    }

    cvar_destroy(table);
    *table = next;
    return true;
}

bool cvar_load_ini(cvar_table *table, const char *ini_path, ini_handler handler,
                   bool force_reload) {
    return cvar_load_internal(table, ini_path, handler, force_reload,
                              (parse_func)ini_parse);
}

bool cvar_load_scf(cvar_table *table, const char *scf_path, scf_handler handler,
                   bool force_reload) {
    return cvar_load_internal(table, scf_path, handler, force_reload,
                              (parse_func)scf_parse);
}
#endif
#endif
