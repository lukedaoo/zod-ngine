#ifndef CVAR_LOAD_H
#define CVAR_LOAD_H

#include "cvar.h"
#include "ini.h"
#include "scf.h"

typedef struct {
    void *min;
    void *max;
} cvar_range;

typedef struct {
    const char *name;  // "section.key"
    cvar_type   expected;
    cvar_range  range;
} cvar_schema_entry;

typedef struct {
    const cvar_schema_entry *entries;
    size_t                   count;
} cvar_schema;

// schema=NULL: infer types only. schema=&s: validate against schema, abort on mismatch.
// force_reload=true: parse into scratch table, swap only on success.
bool cvar_load_ini(cvar_table *table, const char *path, const cvar_schema *schema,
                   bool force_reload);
bool cvar_load_scf(cvar_table *table, const char *path, const cvar_schema *schema,
                   bool force_reload);

// Reason the most recent cvar_load_ini/cvar_load_scf call failed. Empty string if the
// last call succeeded.
const char *cvar_load_get_error(void);

// Merges first then second into out_entries[out_cap].
// Second entries win on name collision. Returns number of entries written.
size_t cvar_schema_merge(const cvar_schema *first, const cvar_schema *second,
                         cvar_schema_entry *out_entries, size_t out_cap);

#ifdef CVAR_LOAD_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static __thread char cvar_load_error_buf[256] = "";

// Reason the most recent cvar_load_ini/cvar_load_scf call failed. Empty string if the
// last call succeeded.
const char *cvar_load_get_error(void) { return cvar_load_error_buf; }

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
static inline bool cvar_check_range(const cvar_schema_entry *e, int type, const void *v) {
    if (!e) return true;

    switch (type) {
        case CVAR_INT: {
            int val = *(const int *)v;
            if (e->range.min && val < *(int *)e->range.min) return false;
            if (e->range.max && val > *(int *)e->range.max) return false;
            return true;
        }
        case CVAR_FLOAT: {
            float val = *(const float *)v;
            if (e->range.min && val < *(float *)e->range.min) return false;
            if (e->range.max && val > *(float *)e->range.max) return false;
            return true;
        }
        default:
            return true;
    }
}

static bool cvar_parse_and_set(const char *section, const char *key, const char *value,
                               cvar_table *table, const cvar_schema_entry *entry) {
    char name[CVAR_NAME_MAX];
    snprintf(name, sizeof(name), "%s.%s", section, key);
    const char *orig = value;

#ifdef MODULE_LOG_ENABLED
#define CVAR_LOAD_DIAG_PRINT() fprintf(stderr, "cvar.load: %s\n", cvar_load_error_buf)
#else
#define CVAR_LOAD_DIAG_PRINT() ((void)0)
#endif

#define CHECK_TYPE(got)                                                           \
    do {                                                                          \
        if (entry && (got) != entry->expected) {                                  \
            snprintf(cvar_load_error_buf, sizeof(cvar_load_error_buf),            \
                     "'%s' expected %s, got %s ('%s')", name,                     \
                     cvar_type_name(entry->expected), cvar_type_name(got), orig); \
            CVAR_LOAD_DIAG_PRINT();                                               \
            return false;                                                         \
        }                                                                         \
    } while (0)

#define CHECK_RANGE(entry, type, val)                                  \
    do {                                                               \
        if (!cvar_check_range((entry), (type), &(val))) {              \
            snprintf(cvar_load_error_buf, sizeof(cvar_load_error_buf), \
                     "'%s' value '%s' is out of range", name, value);  \
            CVAR_LOAD_DIAG_PRINT();                                    \
            return false;                                              \
        }                                                              \
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
                snprintf(cvar_load_error_buf, sizeof(cvar_load_error_buf),
                         "value for '%s' exceeds %zu bytes", name, sizeof(buf) - 1);
                CVAR_LOAD_DIAG_PRINT();
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
                    CHECK_RANGE(entry, CVAR_FLOAT, f);
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
                    CHECK_RANGE(entry, CVAR_INT, v);
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
#ifdef MODULE_LOG_ENABLED
        fprintf(stderr,
                "cvar.load: '%s' has malformed hex literal '%s' — treated as string\n",
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
            CHECK_RANGE(entry, CVAR_INT, ival);
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
            CHECK_RANGE(entry, CVAR_FLOAT, fval);
            return cvar_set_float(table, name, fval);
        }
    }

    CHECK_TYPE(CVAR_STRING);
    return cvar_set_string(table, name, value);

#undef CHECK_TYPE
#undef CHECK_RANGE
#undef CVAR_LOAD_DIAG_PRINT
}

typedef struct {
    cvar_table        *table;
    const cvar_schema *schema;
} cvar_load_ctx;

static const cvar_schema_entry *cvar_schema_lookup(const cvar_schema *schema,
                                                   const char *section, const char *key) {
    if (!schema) return NULL;
    char name[CVAR_NAME_MAX];
    snprintf(name, sizeof(name), "%s.%s", section, key);
    for (size_t i = 0; i < schema->count; i++) {
        if (strcmp(schema->entries[i].name, name) == 0) return &schema->entries[i];
    }
    return NULL;
}

static bool cvar_strict_handler(const char *section, const char *key, const char *value,
                                void *user) {
    cvar_load_ctx           *ctx   = user;
    const cvar_schema_entry *entry = cvar_schema_lookup(ctx->schema, section, key);
    return cvar_parse_and_set(section, key, value, ctx->table, entry);
}

typedef bool (*parse_func)(const char *filepath, void *handler, void *user);

static bool cvar_load_internal(cvar_table *table, const char *path,
                               const cvar_schema *schema, bool force_reload,
                               parse_func parser) {
    cvar_load_error_buf[0] = '\0';

    if (!force_reload) {
        cvar_load_ctx ctx = {.table = table, .schema = schema};
        return parser(path, cvar_strict_handler, &ctx);
    }
    cvar_table    next = {0};
    cvar_load_ctx ctx  = {.table = &next, .schema = schema};
    if (!parser(path, cvar_strict_handler, &ctx)) {
        cvar_destroy(&next);
        return false;
    }
    cvar_destroy(table);
    *table = next;
    return true;
}

bool cvar_load_ini(cvar_table *table, const char *path, const cvar_schema *schema,
                   bool force_reload) {
    return cvar_load_internal(table, path, schema, force_reload, (parse_func)ini_parse);
}

bool cvar_load_scf(cvar_table *table, const char *path, const cvar_schema *schema,
                   bool force_reload) {
    return cvar_load_internal(table, path, schema, force_reload, (parse_func)scf_parse);
}

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

#endif
#endif
