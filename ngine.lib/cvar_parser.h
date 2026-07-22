#ifndef CVAR_PARSER_H
#define CVAR_PARSER_H

#include "cvar.h"
#include "log.h"

#ifndef CVAR_PARSE_STRING_MAX
#define CVAR_PARSE_STRING_MAX 512
#endif

typedef struct {
    cvar_type type;
    union {
        int   i;
        float f;
        bool  b;
        char  str[CVAR_PARSE_STRING_MAX];
    } value;
} cvar_parsed;

// Infers a value string's type in priority order, without touching `table`:
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
// entry == NULL (no schema constraint registered for `name`): infer freely, even if
// `name` already exists with a different type.
// entry != NULL: abort if the inferred type doesn't match entry->expected.
// `table` is read-only here.
bool cvar_parse(cvar_table *table, const char *name, const char *value, cvar_parsed *out);

// cvar_parse() + apply the result via cvar_set_int/float/bool/string — this is
// where the corresponding cvar_set_* range constraint actually gets enforced.
bool cvar_parse_and_set_named(const char *name, const char *value, cvar_table *table);

// Convenience for callers that have section+key instead of a joined name (e.g. an
// INI/SCF "[section] key = value" line) — joins them and calls the above.
bool cvar_parse_and_set(const char *section, const char *key, const char *value,
                        cvar_table *table);

#ifdef CVAR_PARSER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CVAR_PARSER_LOG_ENABLED
#define CVAR_PARSER_LOG_ENABLED 0
#endif

bool cvar_parse(cvar_table *table, const char *name, const char *value,
                cvar_parsed *out) {
    const cvar_constraint *entry = cvar_find_constraint(table, name);

    const char *orig = value;

#define CHECK_TYPE(got)                                                               \
    do {                                                                              \
        if (entry && (got) != entry->expected) {                                      \
            log_error("cvar.parse: '%s' expected %s, got %s ('%s')", name,            \
                      cvar_type_to_string(entry->expected), cvar_type_to_string(got), \
                      orig);                                                          \
            return false;                                                             \
        }                                                                             \
    } while (0)

    size_t len = strlen(value);

    //
    // If the value is wrapped in quotes, remove them. It is a string
    //
    {
        if (len >= 2 && ((value[0] == '\'' && value[len - 1] == '\'') ||
                         (value[0] == '"' && value[len - 1] == '"'))) {
            size_t inner = len - 2;
            if (inner >= sizeof(out->value.str)) {
                log_error("cvar.parse: value for '%s' exceeds %zu bytes", name,
                          sizeof(out->value.str) - 1);
                return false;
            }
            CHECK_TYPE(CVAR_STRING);
            memcpy(out->value.str, value + 1, inner);
            out->value.str[inner] = '\0';
            out->type             = CVAR_STRING;
            return true;
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
                    out->type    = CVAR_FLOAT;
                    out->value.f = f;
                    return true;
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
                    out->type    = CVAR_INT;
                    out->value.i = (int)v;
                    return true;
                }
            }
        }
    }

    //
    // If the value is "true" or "false", try to parse it as a bool
    //
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        CHECK_TYPE(CVAR_BOOL);
        out->type    = CVAR_BOOL;
        out->value.b = strcmp(value, "true") == 0;
        return true;
    }

    char *end;

    //
    // Try to parse it as a hex int (0x prefix — bit-preserving for packed RGBA)
    //
    if (len > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        unsigned long uval = strtoul(value, &end, 16);
        if (end != value && *end == '\0') {
            CHECK_TYPE(CVAR_INT);
            out->type    = CVAR_INT;
            out->value.i = (int)uval;
            return true;
        }
#if CVAR_PARSER_LOG_ENABLED
        log_debug("cvar.parse: '%s' has malformed hex literal '%s' — treated as string",
                  name, orig);
#endif
    }

    //
    // Try to parse it as an int
    //
    {
        long ival = strtol(value, &end, 10);
        if (*value != '\0' && *end == '\0') {
            if (entry && entry->expected == CVAR_FLOAT) {
                out->type    = CVAR_FLOAT;
                out->value.f = (float)ival;
                return true;
            }
            CHECK_TYPE(CVAR_INT);
            out->type    = CVAR_INT;
            out->value.i = (int)ival;
            return true;
        }
    }

    //
    // Try to parse it as a float
    //
    {
        float fval = strtof(value, &end);
        if (end != value && *end == '\0') {
            CHECK_TYPE(CVAR_FLOAT);
            out->type    = CVAR_FLOAT;
            out->value.f = fval;
            return true;
        }
    }

    CHECK_TYPE(CVAR_STRING);
    if (len >= sizeof(out->value.str)) {
        log_error("cvar.parse: value for '%s' exceeds %zu bytes", name,
                  sizeof(out->value.str) - 1);
        return false;
    }
    memcpy(out->value.str, value, len + 1);
    out->type = CVAR_STRING;
    return true;

#undef CHECK_TYPE
}

bool cvar_parse_and_set_named(const char *name, const char *value, cvar_table *table) {
    cvar_parsed p;
    if (!cvar_parse(table, name, value, &p)) return false;

    switch (p.type) {
        case CVAR_INT:
            return cvar_set_int(table, name, p.value.i);
        case CVAR_FLOAT:
            return cvar_set_float(table, name, p.value.f);
        case CVAR_BOOL:
            return cvar_set_bool(table, name, p.value.b);
        case CVAR_STRING:
            return cvar_set_string(table, name, p.value.str);
    }
    return false;
}

bool cvar_parse_and_set(const char *section, const char *key, const char *value,
                        cvar_table *table) {
    char name[CVAR_NAME_MAX];
    int  name_len = snprintf(name, sizeof(name), "%s.%s", section, key);
    if (name_len < 0 || (size_t)name_len >= sizeof(name)) {
        log_error("cvar.parse: '%s.%s' exceeds %d bytes", section, key,
                  CVAR_NAME_MAX - 1);
        return false;
    }
    return cvar_parse_and_set_named(name, value, table);
}

#endif
#endif
