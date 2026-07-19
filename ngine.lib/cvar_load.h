#ifndef CVAR_LOAD_H
#define CVAR_LOAD_H

#include "cvar.h"
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

#ifndef CVAR_LOAD_LOG_ENABLED
#define CVAR_LOAD_LOG_ENABLED 0
#endif

static const char *cvar_type_name(cvar_type t) {
    switch (t) {
        case CVAR_INT:
            return "int";
        case CVAR_FLOAT:
            return "float";
        case CVAR_BOOL:
            return "bool";
        case CVAR_STRING:
            return "string";
    }
    return "unknown";
}

// Maps "section.key = value" into a cvar, inferring type in priority order:
//
//   'val' or "val"         CVAR_STRING  (quotes stripped, type forced)
//   1.5F / 1.5f / '1.5F    CVAR_FLOAT   (explicit float suffix)
//   42L  / 42l /  '1.5L    CVAR_INT     (explicit int suffix)
//   true / false           CVAR_BOOL
//   0xFF / 0xDEAD          CVAR_INT     (0x prefix, strtoul, bit-preserving cast)
//   42                     CVAR_INT     (strtol, whole string consumed)
//   1.5                    CVAR_FLOAT   (strtof, whole string consumed)
//   anything else          CVAR_STRING
//
// entry == NULL: infer only. entry != NULL: abort if inferred type != entry->expected.
// Range enforcement happens inside cvar_set_int/cvar_set_float themselves (via the
// constraints attached to `table`), not here.
static bool cvar_parse_and_set(const char *section, const char *key, const char *value,
                               cvar_table *table, const cvar_constraint *entry) {
    char name[CVAR_NAME_MAX];
    int  name_len = snprintf(name, sizeof(name), "%s.%s", section, key);
    if (name_len < 0 || (size_t)name_len >= sizeof(name)) {
        log_error("cvar.load: '%s.%s' exceeds %d bytes", section, key, CVAR_NAME_MAX - 1);
        return false;
    }
    const char *orig = value;

#define CHECK_TYPE(got)                                                            \
    do {                                                                           \
        if (entry && (got) != entry->expected) {                                   \
            log_error("cvar.load: '%s' expected %s, got %s ('%s')", name,          \
                      cvar_type_name(entry->expected), cvar_type_name(got), orig); \
            return false;                                                          \
        }                                                                          \
    } while (0)

    size_t len = strlen(value);

    //
    // If the value is wrapped in quotes, remove them. It is a string
    //
    {
        if (len >= 2 && ((value[0] == '\'' && value[len - 1] == '\'') ||
                         (value[0] == '"' && value[len - 1] == '"'))) {
            char   buf[512];
            size_t inner = len - 2;
            if (inner >= sizeof(buf)) {
                log_error("cvar.load: value for '%s' exceeds %zu bytes", name,
                          sizeof(buf) - 1);
                return false;
            }
            memcpy(buf, value + 1, inner);
            buf[inner] = '\0';
            CHECK_TYPE(CVAR_STRING);
            return cvar_set_string(table, name, buf);
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
        bool is_hex = len > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X');

        if (!is_hex && (last == 'f' || last == 'F')) {
            size_t nlen = len - 1;
            if (nlen > 0 && nlen < sizeof(buf)) {
                memcpy(buf, value, nlen);
                buf[nlen] = '\0';
                char *end;
                float f = strtof(buf, &end);
                if (end != buf && *end == '\0') {
                    CHECK_TYPE(CVAR_FLOAT);
                    return cvar_set_float(table, name, f);
                }
            }
        }

        if (!is_hex && (last == 'l' || last == 'L')) {
            size_t nlen = len - 1;
            if (nlen > 0 && nlen < sizeof(buf)) {
                memcpy(buf, value, nlen);
                buf[nlen] = '\0';
                char *end;
                long  v = strtol(buf, &end, 10);
                if (end != buf && *end == '\0') {
                    CHECK_TYPE(CVAR_INT);
                    return cvar_set_int(table, name, (int)v);
                }
            }
        }
    }

    //
    // If the value is "true" or "false", try to parse it as a bool
    //
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        CHECK_TYPE(CVAR_BOOL);
        return cvar_set_bool(table, name, strcmp(value, "true") == 0);
    }

    char *end;

    //
    // Try to parse it as a hex int (0x prefix — bit-preserving for packed RGBA)
    //
    if (len > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        unsigned long uval = strtoul(value, &end, 16);
        if (end != value && *end == '\0') {
            CHECK_TYPE(CVAR_INT);
            return cvar_set_int(table, name, (int)uval);
        }
#if CVAR_LOAD_LOG_ENABLED
        log_debug("cvar.load: '%s' has malformed hex literal '%s' — treated as string",
                  name, orig);
#endif
    }

    //
    // Try to parse it as an int
    //
    {
        long ival = strtol(value, &end, 10);
        if (*value != '\0' && *end == '\0') {
            CHECK_TYPE(CVAR_INT);
            return cvar_set_int(table, name, (int)ival);
        }
    }

    //
    // Try to parse it as a float
    //
    {
        float fval = strtof(value, &end);
        if (end != value && *end == '\0') {
            CHECK_TYPE(CVAR_FLOAT);
            return cvar_set_float(table, name, fval);
        }
    }

    CHECK_TYPE(CVAR_STRING);
    return cvar_set_string(table, name, value);

#undef CHECK_TYPE
}

static const cvar_constraint *cvar_load_lookup(const cvar_table *table,
                                               const char *section, const char *key) {
    char name[CVAR_NAME_MAX];
    int  name_len = snprintf(name, sizeof(name), "%s.%s", section, key);
    if (name_len < 0 || (size_t)name_len >= sizeof(name)) return NULL;
    return cvar_find_constraint(table, name);
}

static bool cvar_strict_handler(const char *section, const char *key, const char *value,
                                void *user) {
    cvar_table            *table = user;
    const cvar_constraint *entry = cvar_load_lookup(table, section, key);
    return cvar_parse_and_set(section, key, value, table, entry);
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
