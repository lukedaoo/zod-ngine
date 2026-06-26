#ifndef CARG_H
#define CARG_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { CARG_INT, CARG_FLOAT, CARG_BOOL, CARG_STRING } carg_type;

typedef struct {
    const char *flag;
    size_t      arg_count;  // 0 => bool flag, no value consumed
    carg_type   type;       // ignored when arg_count == 0
    bool        required;
} carg_register_t;

typedef struct carg_table carg_table;
typedef struct carg_t     carg_t;

bool carg_parse(const carg_register_t *defs,
                const size_t           ndefs,
                const int              argc,
                const char           **argv,
                carg_table            *table);
void carg_destroy(carg_table *table);

carg_t *carg_get(carg_table *table, const char *flag);

bool         carg_get_bool(carg_table *t, const char *flag, bool fallback);
const int   *carg_get_int_array(carg_table *t, const char *flag, size_t *count);
const float *carg_get_float_array(carg_table *t, const char *flag, size_t *count);
const char **carg_get_string_array(carg_table *t, const char *flag, size_t *count);

#ifdef CARG_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct carg_t {
    const char *flag;
    carg_type   type;
    bool        present;
    size_t      count;
    union {
        int         *i;
        float       *f;
        bool         b;
        const char **s;
    } value;
};

struct carg_table {
    carg_t *data;
    size_t  size;
};

static void carg_entry_free_value(carg_t *e) {
    if (e->type == CARG_BOOL) return;
    switch (e->type) {
        case CARG_INT:
            free(e->value.i);
            break;
        case CARG_FLOAT:
            free(e->value.f);
            break;
        case CARG_STRING:
            free((void *)e->value.s);
            break;
        default:
            break;
    }
}

void carg_destroy(carg_table *table) {
    if (!table) return;
    for (size_t i = 0; i < table->size; i++) carg_entry_free_value(&table->data[i]);
    free(table->data);
    table->data = NULL;
    table->size = 0;
}

carg_t *carg_get(carg_table *table, const char *flag) {
    if (!table || !table->data || !flag) return NULL;
    for (size_t i = 0; i < table->size; i++) {
        if (strcmp(table->data[i].flag, flag) == 0) return &table->data[i];
    }
    return NULL;
}

static const carg_register_t *carg_find_def(const carg_register_t *defs,
                                            size_t                 ndefs,
                                            const char            *flag) {
    for (size_t i = 0; i < ndefs; i++) {
        if (strcmp(defs[i].flag, flag) == 0) return &defs[i];
    }
    return NULL;
}

bool carg_parse(const carg_register_t *defs,
                const size_t           ndefs,
                const int              argc,
                const char           **argv,
                carg_table            *table) {
    if (!defs || !table || !argv) return false;

    //
    // Ensure all flags start with `--`
    //
    {
        for (size_t i = 0; i < ndefs; i++) {
            const char *flag = defs[i].flag;
            if (!flag || flag[0] != '-' || flag[1] != '-') {
#ifdef MODULE_LOG_ENABLED
                fprintf(stderr,
                        "carg.carg_parse: Flag '%s' must start with `--`; got '%s'.\n",
                        flag ? flag : "(null)", flag ? flag : "(null)");
#endif
                return false;
            }
        }
    }

    table->data = malloc(ndefs * sizeof(carg_t));
    if (!table->data) return false;
    table->size = ndefs;

    for (size_t i = 0; i < ndefs; i++) {
        table->data[i] = (carg_t){
             .flag    = defs[i].flag,
             .type    = defs[i].type,
             .present = false,
             .count   = 0,
             .value   = {0},
        };
    }

    for (int a = 1; a < argc; a++) {
        const carg_register_t *def = carg_find_def(defs, ndefs, argv[a]);
        if (!def) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: unknown flag %s\n", argv[a]);
#endif
            carg_destroy(table);
            return false;
        }
        carg_t *e = carg_get(table, def->flag);

        if (def->arg_count == 0) {
            e->present = true;
            e->value.b = true;
            e->count   = 0;
            continue;
        }

        if ((size_t)(argc - a - 1) < def->arg_count) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: %s requires %zu value(s)\n", def->flag,
                    def->arg_count);
#endif
            carg_destroy(table);
            return false;
        }

        carg_entry_free_value(e);

        switch (def->type) {
            case CARG_INT: {
                int *vals = malloc(def->arg_count * sizeof(int));
                if (!vals) {
                    carg_destroy(table);
                    return false;
                }
                for (size_t k = 0; k < def->arg_count; k++) {
                    const char *tok = argv[a + 1 + k];
                    char       *end;
                    long        v = strtol(tok, &end, 10);
                    if (*end != '\0' || end == tok) {
#ifdef MODULE_LOG_ENABLED
                        fprintf(stderr, "carg.carg_parse: %s: invalid int value '%s'\n",
                                def->flag, tok);
#endif
                        free(vals);
                        carg_destroy(table);
                        return false;
                    }
                    vals[k] = (int)v;
                }
                e->value.i = vals;
                break;
            }
            case CARG_FLOAT: {
                float *vals = malloc(def->arg_count * sizeof(float));
                if (!vals) {
                    carg_destroy(table);
                    return false;
                }
                for (size_t k = 0; k < def->arg_count; k++) {
                    const char *tok = argv[a + 1 + k];
                    char       *end;
                    float       v = strtof(tok, &end);
                    if (*end != '\0' || end == tok) {
#ifdef MODULE_LOG_ENABLED
                        fprintf(stderr, "carg.carg_parse: %s: invalid float value '%s'\n",
                                def->flag, tok);
#endif
                        free(vals);
                        carg_destroy(table);
                        return false;
                    }
                    vals[k] = v;
                }
                e->value.f = vals;
                break;
            }
            case CARG_STRING: {
                const char **vals =
                     (const char **)malloc(def->arg_count * sizeof(const char *));
                if (!vals) {
                    carg_destroy(table);
                    return false;
                }
                for (size_t k = 0; k < def->arg_count; k++) vals[k] = argv[a + 1 + k];
                e->value.s = vals;
                break;
            }
            case CARG_BOOL:
            default:
#ifdef MODULE_LOG_ENABLED
                fprintf(stderr, "carg.carg_parse: %s: bool flags must have arg_count 0\n",
                        def->flag);
#endif

                carg_destroy(table);
                return false;
        }

        e->present = true;
        e->count   = def->arg_count;
        a += (int)def->arg_count;
    }

    for (size_t i = 0; i < ndefs; i++) {
        if (defs[i].required && !table->data[i].present) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: missing required flag %s\n", defs[i].flag);
#endif
            carg_destroy(table);
            return false;
        }
    }

    return true;
}

bool carg_get_bool(carg_table *t, const char *flag, bool fallback) {
    carg_t *e = carg_get(t, flag);
    if (!e || e->type != CARG_BOOL || !e->present) return fallback;
    return e->value.b;
}

const int *carg_get_int_array(carg_table *t, const char *flag, size_t *count) {
    carg_t *e = carg_get(t, flag);
    if (!e || e->type != CARG_INT || !e->present) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = e->count;
    return e->value.i;
}

const float *carg_get_float_array(carg_table *t, const char *flag, size_t *count) {
    carg_t *e = carg_get(t, flag);
    if (!e || e->type != CARG_FLOAT || !e->present) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = e->count;
    return e->value.f;
}

const char **carg_get_string_array(carg_table *t, const char *flag, size_t *count) {
    carg_t *e = carg_get(t, flag);
    if (!e || e->type != CARG_STRING || !e->present) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = e->count;
    return e->value.s;
}

#endif
#endif
