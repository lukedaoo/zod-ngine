#ifndef CVAR_LOAD_H
#define CVAR_LOAD_H

#include "cvar.h"
#include "ini.h"
#include "scf.h"

bool cvar_load_ini(cvar_table *table, const char *ini_path, ini_handler handler,
                   bool force_reload);

bool cvar_load_scf(cvar_table *table, const char *scf_path, scf_handler handler,
                   bool force_reload);

// Maps "section.key = value" into a cvar, inferring type in priority order:
//
//   'val' or "val"   CVAR_STRING (quotes stripped, type forced)
//   '1.5F or '1.5f   CVAR_FLOAT  (unmatched leading quote + float suffix)
//   '42L  or '42l    CVAR_INT    (unmatched leading quote + int suffix)
//   1.5F / 1.5f      CVAR_FLOAT  (explicit float suffix, no quote)
//   42L  / 42l       CVAR_INT    (explicit int suffix, no quote)
//   true / false     CVAR_BOOL
//   42               CVAR_INT    (strtol, whole string consumed)
//   1.5              CVAR_FLOAT  (strtof, whole string consumed)
//   anything else    CVAR_STRING
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

    size_t len = strlen(value);

    //
    // If the value is wrapped in quotes, remove them. It is a string
    //
    {
        if (len >= 2 && ((value[0] == '\'' && value[len - 1] == '\'') ||
                         (value[0] == '"' && value[len - 1] == '"'))) {
            char   buf[512];
            size_t inner = len - 2;
            if (inner >= sizeof(buf)) return false;
            memcpy(buf, value + 1, inner);
            buf[inner] = '\0';
            return cvar_set_string(cvars, name, buf);
        }
    }

    if (value[0] == '\'' || value[0] == '"') {
        value++;
        len--;
    }

    //
    // If the value ends with 'f' or 'l', try to parse it as a float or int
    //
    if (len > 0) {
        char last = value[len - 1];
        char buf[128];

        if (last == 'f' || last == 'F') {
            size_t nlen = len - 1;
            if (nlen > 0 && nlen < sizeof(buf)) {
                memcpy(buf, value, nlen);
                buf[nlen] = '\0';
                char *end;
                float f = strtof(buf, &end);
                if (end != buf && *end == '\0') return cvar_set_float(cvars, name, f);
            }
        }

        if (last == 'l' || last == 'L') {
            size_t nlen = len - 1;
            if (nlen > 0 && nlen < sizeof(buf)) {
                memcpy(buf, value, nlen);
                buf[nlen] = '\0';
                char *end;
                long  v = strtol(buf, &end, 10);
                if (end != buf && *end == '\0') return cvar_set_int(cvars, name, (int)v);
            }
        }
    }

    //
    // If the value is "true" or "false", try to parse it as a bool
    //
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0)
        return cvar_set_bool(cvars, name, strcmp(value, "true") == 0);

    char *end;

    //
    // Try to parse it as an int
    //
    {
        long ival = strtol(value, &end, 10);
        if (*value != '\0' && *end == '\0') return cvar_set_int(cvars, name, (int)ival);
    }

    //
    // Try to parse it as a float
    //
    {
        float fval = strtof(value, &end);
        if (end != value && *end == '\0') return cvar_set_float(cvars, name, fval);
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
