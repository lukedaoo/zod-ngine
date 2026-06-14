#ifndef CVAR_LOAD_H
#define CVAR_LOAD_H
#include "cvar.h"
#include "ini.h"

#include <stdbool.h>

bool cvar_load_ini(cvar_table *table, const char *ini_path, ini_handler handler);

// Generic ini_handler: maps any "[section] key = value" to a cvar named
// "section.key", inferring CVAR_BOOL/CVAR_INT/CVAR_FLOAT/CVAR_STRING from
// the value text.
bool cvar_default_ini_handler(const char *section,
                              const char *key,
                              const char *value,
                              void       *user);

#ifdef CVAR_LOAD_IMPLEMENTATION
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool cvar_default_ini_handler(const char *section,
                              const char *key,
                              const char *value,
                              void       *user) {
    cvar_table *cvars = user;

    char name[CVAR_NAME_MAX];
    snprintf(name, sizeof(name), "%s.%s", section, key);

    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        bool b = strcmp(value, "true") == 0;
        return cvar_set(cvars, name, CVAR_BOOL, &b);
    }

    char *end;
    long  ival = strtol(value, &end, 10);
    if (*value != '\0' && *end == '\0') {
        int i = (int)ival;
        return cvar_set(cvars, name, CVAR_INT, &i);
    }

    float fval = strtof(value, &end);
    if (*value != '\0' && *end == '\0') {
        return cvar_set(cvars, name, CVAR_FLOAT, &fval);
    }

    return cvar_set(cvars, name, CVAR_STRING, (void *)value);
}

bool cvar_load_ini(cvar_table *table,
                   const char *ini_path,
                   ini_handler handler) {
    cvar_table next = {0};
    if (!ini_parse(ini_path, handler, &next) || next.size == 0) {
        log_error("cvar_load_ini: failed to parse %s, keeping previous config",
                  ini_path);
        cvar_destroy(&next);
        return false;
    }

    cvar_destroy(table);
    *table = next;
    return true;
}
#endif
#endif
