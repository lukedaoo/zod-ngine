#ifndef CARG_H
#define CARG_H

#include <stdbool.h>
#include <stddef.h>

/*
 * @info(cargs) — command-line argument parser
 *
 * 1. Define a static array of carg_register (flag, arg_count, type, required).
 *    arg_count == 0  ->  bool flag, no values consumed.
 *    All flags must start with `--`.
 *
 * 2. Call carg_parse(). Returns false on error (unknown flag, wrong type,
 *    too few values, too many values, missing required).
 *    Enable MODULE_LOG_ENABLED for stderr output.
 *
 * 3. Read values with typed getters. Array getters return NULL when absent.
 *    Bool getter returns fallback when absent.
 *
 * 4. Call carg_destroy() when done.
 *
 * Example:
 *   carg_register defs[] = {
 *       {.flag="--size", .arg_count=2, .type=CARG_INT,  .required=true },
 *       {.flag="--log",  .arg_count=0, .type=CARG_BOOL, .required=false},
 *   };
 *   carg_table table = {0};
 *   if (!carg_parse(defs, 2, argc, (const char **)argv, &table)) return 1;
 *   size_t n;
 *   const int *sz = carg_get_int_array(&table, "--size", &n); // n==2
 *   bool log      = carg_get_bool(&table, "--log", false);
 *   carg_destroy(&table);
 */

typedef enum { CARG_INT, CARG_FLOAT, CARG_BOOL, CARG_STRING } carg_type;

typedef struct {
    const char *flag;
    size_t      arg_count;
    carg_type   type;
    bool        required;
} carg_register;

typedef struct carg_table carg_table;
typedef struct carg     carg;

bool carg_parse(const carg_register *defs, const size_t ndefs, const int argc,
                const char **argv, carg_table *table);
void carg_destroy(carg_table *table);

carg *carg_get(carg_table *table, const char *flag);

bool         carg_get_bool(carg_table *t, const char *flag, bool fallback);
const int   *carg_get_int_array(carg_table *t, const char *flag, size_t *count);
const float *carg_get_float_array(carg_table *t, const char *flag, size_t *count);
const char **carg_get_string_array(carg_table *t, const char *flag, size_t *count);

#ifdef CARG_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct carg {
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
    carg *data;
    size_t  size;
};

static void carg_entry_free_value(carg *e) {
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

carg *carg_get(carg_table *table, const char *flag) {
    if (!table || !table->data || !flag) return NULL;
    for (size_t i = 0; i < table->size; i++) {
        if (strcmp(table->data[i].flag, flag) == 0) return &table->data[i];
    }
    return NULL;
}

static const carg_register *carg_find_def(const carg_register *defs, size_t ndefs,
                                            const char *flag) {
    for (size_t i = 0; i < ndefs; i++) {
        if (strcmp(defs[i].flag, flag) == 0) return &defs[i];
    }
    return NULL;
}

static int *carg_parse_ints([[maybe_unused]] const char *flag, const char **argv,
                            int base, size_t n) {
    int *vals = malloc(n * sizeof(int));
    if (!vals) return NULL;
    for (size_t k = 0; k < n; k++) {
        const char *tok = argv[base + (int)k];
        char       *end;
        long        v = strtol(tok, &end, 10);
        if (*end != '\0' || end == tok) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: %s: invalid int value '%s'\n", flag, tok);
#endif
            free(vals);
            return NULL;
        }
        vals[k] = (int)v;
    }
    return vals;
}

static float *carg_parse_floats([[maybe_unused]] const char *flag, const char **argv,
                                int base, size_t n) {
    float *vals = malloc(n * sizeof(float));
    if (!vals) return NULL;
    for (size_t k = 0; k < n; k++) {
        const char *tok = argv[base + (int)k];
        char       *end;
        float       v = strtof(tok, &end);
        if (*end != '\0' || end == tok) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: %s: invalid float value '%s'\n", flag, tok);
#endif
            free(vals);
            return NULL;
        }
        vals[k] = v;
    }
    return vals;
}

bool carg_parse(const carg_register *defs, const size_t ndefs, const int argc,
                const char **argv, carg_table *table) {
    if (!defs || !table || !argv) return false;

    //
    // Check that all flags start with `--`.
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

    //
    // Allocate table.
    //
    {
        table->data = malloc(ndefs * sizeof(carg));
        if (!table->data) return false;
        table->size = ndefs;

        for (size_t i = 0; i < ndefs; i++) {
            table->data[i] = (carg){
                 .flag    = defs[i].flag,
                 .type    = defs[i].type,
                 .present = false,
                 .count   = 0,
                 .value   = {0},
            };
        }
    }

    // parsing
    for (int a = 1; a < argc; a++) {
        const carg_register *def = carg_find_def(defs, ndefs, argv[a]);
        if (!def) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: unknown flag %s\n", argv[a]);
#endif
            carg_destroy(table);
            return false;
        }
        carg *e = carg_get(table, def->flag);

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
                e->value.i = carg_parse_ints(def->flag, argv, a + 1, def->arg_count);
                if (!e->value.i) {
                    carg_destroy(table);
                    return false;
                }
                break;
            }
            case CARG_FLOAT: {
                e->value.f = carg_parse_floats(def->flag, argv, a + 1, def->arg_count);
                if (!e->value.f) {
                    carg_destroy(table);
                    return false;
                }
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

        //
        // enfore that arg have exactly `arg_count`
        //
        {
            size_t excess = 0;
            for (int j = a + 1; j < argc; j++) {
                if (argv[j][0] == '-' && argv[j][1] == '-') break;
                excess++;
            }
            if (excess > 0) {
#ifdef MODULE_LOG_ENABLED
                fprintf(stderr, "carg.carg_parse: %s: got %zu value(s), expected %zu\n",
                        def->flag, def->arg_count + excess, def->arg_count);
#endif
                carg_destroy(table);
                return false;
            }
        }
    }

    for (size_t i = 0; i < ndefs; i++) {
        if (defs[i].required && !table->data[i].present) {
#ifdef MODULE_LOG_ENABLED
            fprintf(stderr, "carg.carg_parse: missing required flag %s\n", defs[i].flag);
#endif
            carg_destroy(table);
            return false;
        }

#ifdef MODULE_LOG_ENABLED
        fprintf(stderr, "carg.carg_parse: parsed arg %s\n", defs[i].flag);
#endif
    }
    return true;
}

bool carg_get_bool(carg_table *t, const char *flag, bool fallback) {
    carg *e = carg_get(t, flag);
    if (!e || e->type != CARG_BOOL || !e->present) return fallback;
    return e->value.b;
}

const int *carg_get_int_array(carg_table *t, const char *flag, size_t *count) {
    carg *e = carg_get(t, flag);
    if (!e || e->type != CARG_INT || !e->present) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = e->count;
    return e->value.i;
}

const float *carg_get_float_array(carg_table *t, const char *flag, size_t *count) {
    carg *e = carg_get(t, flag);
    if (!e || e->type != CARG_FLOAT || !e->present) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = e->count;
    return e->value.f;
}

const char **carg_get_string_array(carg_table *t, const char *flag, size_t *count) {
    carg *e = carg_get(t, flag);
    if (!e || e->type != CARG_STRING || !e->present) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = e->count;
    return e->value.s;
}

#endif
#endif
