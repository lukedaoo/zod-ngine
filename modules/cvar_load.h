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
bool cvar_infer_handler(const char *section, const char *key, const char *value,
                        void *user);

typedef struct {
    const char *name;  // "section.key"
    cvar_type   expected;
} cvar_schema_entry;

typedef struct {
    const cvar_schema_entry *entries;
    size_t                   count;
} cvar_schema;

typedef struct {
    cvar_table        *table;
    const cvar_schema *schema;  // NULL = untyped (falls through to default handler)
} cvar_load_ctx;

// Merges first then second into out_entries[out_cap].
// Second entries win on name collision. Returns number of entries written.
size_t cvar_schema_merge(const cvar_schema *first, const cvar_schema *second,
                         cvar_schema_entry *out_entries, size_t out_cap);

// Like cvar_infer_handler but validates inferred type against ctx->schema
// before storing. Returns false (aborts parse) on type mismatch.
// user must be cvar_load_ctx *.
bool cvar_strict_handler(const char *section, const char *key, const char *value,
                         void *user);

bool cvar_load_scf_typed(cvar_table *table, const char *path, const cvar_schema *schema,
                         bool force_reload);
bool cvar_load_ini_typed(cvar_table *table, const char *path, const cvar_schema *schema,
                         bool force_reload);

#ifdef CVAR_LOAD_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool cvar_infer_handler(const char *section, const char *key, const char *value,
                        void *user) {
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
            if (inner >= sizeof(buf)) {
#ifdef MODULE_LOG_ENABLED
                fprintf(stderr,
                        "cvar.load: value for '%s' exceeds %zu bytes — truncate or "
                        "shorten the value\n",
                        name, sizeof(buf) - 1);
#endif
                return false;
            }
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

// ---- schema helpers ----

static const cvar_schema_entry *cvar_schema_lookup(const cvar_schema *schema,
                                                   const char        *name) {
    if (!schema) return NULL;
    for (size_t i = 0; i < schema->count; i++) {
        if (strcmp(schema->entries[i].name, name) == 0) return &schema->entries[i];
    }
    return NULL;
}

static cvar_type cvar_infer_type(const char *value) {
    size_t len = strlen(value);

    if (len >= 2 && ((value[0] == '\'' && value[len - 1] == '\'') ||
                     (value[0] == '"' && value[len - 1] == '"')))
        return CVAR_STRING;

    if (value[0] == '\'' || value[0] == '"') {
        value++;
        len--;
    }

    if (len > 0) {
        char   last = value[len - 1];
        char   buf[128];
        char  *end;
        size_t nlen = len - 1;

        if ((last == 'f' || last == 'F') && nlen > 0 && nlen < sizeof(buf)) {
            memcpy(buf, value, nlen);
            buf[nlen] = '\0';
            strtof(buf, &end);
            if (end != buf && *end == '\0') return CVAR_FLOAT;
        }
        if ((last == 'l' || last == 'L') && nlen > 0 && nlen < sizeof(buf)) {
            memcpy(buf, value, nlen);
            buf[nlen] = '\0';
            strtol(buf, &end, 10);
            if (end != buf && *end == '\0') return CVAR_INT;
        }
    }

    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) return CVAR_BOOL;

    char *end;
    if (*value != '\0') {
        strtol(value, &end, 10);
        if (*end == '\0') return CVAR_INT;
    }

    {
        float f = strtof(value, &end);
        (void)f;
        if (end != value && *end == '\0') return CVAR_FLOAT;
    }

    return CVAR_STRING;
}

#ifdef MODULE_LOG_ENABLED
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
#endif

// @perf(cvar_schema_merge): O(n^2) when the count is small, this is fine.
// but when the count is large (>200), use hashmap
size_t cvar_schema_merge(const cvar_schema *first, const cvar_schema *second,
                         cvar_schema_entry *out, size_t cap) {
    size_t n = 0;

    if (first) {
        for (size_t i = 0; i < first->count; i++) {
            if (n >= cap) {
#ifdef MODULE_LOG_ENABLED
                fprintf(stderr,
                        "cvar.schema_merge: (first schema) buffer full (%zu), dropping "
                        "'%s' — increase "
                        "cap\n",
                        cap, first->entries[i].name);
#endif
                break;
            }
            out[n++] = first->entries[i];
        }
    }

    if (second) {
        for (size_t i = 0; i < second->count; i++) {
            const char *name  = second->entries[i].name;
            bool        found = false;
            // replace
            for (size_t j = 0; j < n; j++) {
                if (strcmp(out[j].name, name) == 0) {
                    out[j] = second->entries[i];
                    found  = true;
                    break;
                }
            }
            if (!found) {
                if (n >= cap) {
#ifdef MODULE_LOG_ENABLED
                    fprintf(stderr,
                            "cvar.schema_merge: (second schema) buffer full (%zu), "
                            "dropping '%s' — increase "
                            "cap\n",
                            cap, first->entries[i].name);
#endif
                    continue;
                }
                out[n++] = second->entries[i];
            }
        }
    }

    return n;
}

bool cvar_strict_handler(const char *section, const char *key, const char *value,
                         void *user) {
    cvar_load_ctx *ctx = user;

    if (ctx->schema) {
        char name[CVAR_NAME_MAX];
        snprintf(name, sizeof(name), "%s.%s", section, key);

        const cvar_schema_entry *entry = cvar_schema_lookup(ctx->schema, name);
        if (entry) {
            cvar_type inferred = cvar_infer_type(value);
            if (inferred != entry->expected) {
#ifdef MODULE_LOG_ENABLED
                fprintf(stderr,
                        "cvar.load: '%s' expected %s, got %s ('%s') — aborting load\n",
                        name, cvar_type_name(entry->expected), cvar_type_name(inferred),
                        value);
#endif
                return false;
            }
        }
    }

    return cvar_infer_handler(section, key, value, ctx->table);
}

bool cvar_load_scf_typed(cvar_table *table, const char *path, const cvar_schema *schema,
                         bool force_reload) {
    if (!force_reload) {
        cvar_load_ctx ctx = {.table = table, .schema = schema};
        return scf_parse(path, cvar_strict_handler, &ctx);
    }

    cvar_table    next = {0};
    cvar_load_ctx ctx  = {.table = &next, .schema = schema};
    if (!scf_parse(path, cvar_strict_handler, &ctx)) {
        cvar_destroy(&next);
        return false;
    }
    cvar_destroy(table);
    *table = next;
    return true;
}

bool cvar_load_ini_typed(cvar_table *table, const char *path, const cvar_schema *schema,
                         bool force_reload) {
    if (!force_reload) {
        cvar_load_ctx ctx = {.table = table, .schema = schema};
        return ini_parse(path, cvar_strict_handler, &ctx);
    }

    cvar_table    next = {0};
    cvar_load_ctx ctx  = {.table = &next, .schema = schema};
    if (!ini_parse(path, cvar_strict_handler, &ctx)) {
        cvar_destroy(&next);
        return false;
    }
    cvar_destroy(table);
    *table = next;
    return true;
}

#endif
#endif
