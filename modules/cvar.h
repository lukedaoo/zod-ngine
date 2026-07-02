#ifndef CVAR_H
#define CVAR_H

#include <stdbool.h>
#include <stddef.h>

#include "log.h"

typedef enum { CVAR_INT, CVAR_FLOAT, CVAR_BOOL, CVAR_STRING } cvar_type;

typedef struct cvar_table cvar_table;
typedef struct cvar       cvar;

void cvar_destroy(cvar_table *table);

cvar *cvar_get(cvar_table *table, const char *name);

int         cvar_get_int(cvar_table *t, const char *name, int fallback);
float       cvar_get_float(cvar_table *t, const char *name, float fallback);
bool        cvar_get_bool(cvar_table *t, const char *name, bool fallback);
const char *cvar_get_string(cvar_table *t, const char *name, const char *fallback);

bool cvar_set_int(cvar_table *t, const char *name, int val);
bool cvar_set_float(cvar_table *t, const char *name, float val);
bool cvar_set_bool(cvar_table *t, const char *name, bool val);
bool cvar_set_string(cvar_table *t, const char *name, const char *val);

void cvar_print(const cvar_table *table);

typedef struct {
    bool has_min;
    bool has_max;
    union {
        int   i;
        float f;
    } min;
    union {
        int   i;
        float f;
    } max;
} cvar_range;

typedef struct {
    const char *name;
    cvar_type   expected;
    cvar_range  range;
} cvar_constraint;

typedef struct {
    cvar_constraint *entries;
    size_t           size;
} cvar_schema;

bool cvar_add_schema(cvar_table *table, const cvar_constraint *entries, size_t count);

const cvar_constraint *cvar_find_constraint(const cvar_table *table, const char *name);

bool cvar_copy_schema(cvar_table *dest, const cvar_table *src);

#ifdef CVAR_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CVAR_TABLE_MAX_SIZE
#define CVAR_TABLE_MAX_SIZE 1024
#endif

#ifndef CVAR_DEFAULT_CAPACITY
#define CVAR_DEFAULT_CAPACITY 8
#endif

#ifndef CVAR_NAME_MAX
#define CVAR_NAME_MAX 64
#endif

#ifndef CVAR_LOG_ENABLED
#define CVAR_LOG_ENABLED 0
#endif

struct cvar {
    char      name[CVAR_NAME_MAX];
    cvar_type type;
    union {
        int   i;
        float f;
        bool  b;
        struct {
            char  *data;
            size_t cap;
        } str;
    } value;
};

struct cvar_table {
    cvar  *data;
    size_t size;
    size_t capacity;

    cvar_schema schema;
};

const cvar_constraint *cvar_find_constraint(const cvar_table *table, const char *name) {
    if (!table) return NULL;
    for (size_t i = 0; i < table->schema.size; i++) {
        if (strcmp(table->schema.entries[i].name, name) == 0)
            return &table->schema.entries[i];
    }
    return NULL;
}

bool cvar_add_schema(cvar_table *table, const cvar_constraint *entries, size_t count) {
    if (!table) return false;
    if (!entries || count == 0) return true;

    size_t           new_size = table->schema.size + count;
    cvar_constraint *new_data =
         realloc(table->schema.entries, new_size * sizeof(cvar_constraint));
    if (!new_data) return false;
    table->schema.entries = new_data;

    bool ok = true;
    for (size_t i = 0; i < count; i++) {
        if (cvar_find_constraint(table, entries[i].name)) {
            log_error("cvar.add_schema: '%s' already has a constraint — ignoring",
                      entries[i].name);
            ok = false;
            continue;
        }
        table->schema.entries[table->schema.size++] = entries[i];
    }
    return ok;
}

bool cvar_copy_schema(cvar_table *dest, const cvar_table *src) {
    if (!dest || !src) return false;
    if (src->schema.size == 0) return true;

    free(dest->schema.entries);
    dest->schema.entries = malloc(src->schema.size * sizeof(cvar_constraint));
    if (!dest->schema.entries) {
        dest->schema.size = 0;
        return false;
    }

    memcpy(dest->schema.entries, src->schema.entries,
           src->schema.size * sizeof(cvar_constraint));
    dest->schema.size = src->schema.size;
    return true;
}

static bool cvar_range_ok(const cvar_constraint *c, cvar_type type, const void *value) {
    if (!c || c->expected != type) return true;
    switch (type) {
        case CVAR_INT: {
            int v = *(const int *)value;
            if (c->range.has_min && v < c->range.min.i) return false;
            if (c->range.has_max && v > c->range.max.i) return false;
            return true;
        }
        case CVAR_FLOAT: {
            float v = *(const float *)value;
            if (c->range.has_min && v < c->range.min.f) return false;
            if (c->range.has_max && v > c->range.max.f) return false;
            return true;
        }
        default:
            return true;
    }
}

void cvar_destroy(cvar_table *table) {
    if (!table) return;
    for (size_t i = 0; i < table->size; i++) {
        if (table->data[i].type == CVAR_STRING) free(table->data[i].value.str.data);
    }
    free(table->data);
    table->data     = NULL;
    table->size     = 0;
    table->capacity = 0;

    free(table->schema.entries);
    table->schema.entries = NULL;
    table->schema.size    = 0;
}

cvar *cvar_get(cvar_table *table, const char *name) {
    if (!table || !table->data || !name) return NULL;
    for (size_t i = 0; i < table->size; i++) {
        if (strcmp(table->data[i].name, name) == 0) return &table->data[i];
    }
    return NULL;
}

int cvar_get_int(cvar_table *t, const char *name, int fallback) {
    cvar *cv = cvar_get(t, name);
    if (!cv || cv->type != CVAR_INT) {
#if CVAR_LOG_ENABLED
        log_debug(
             "cvar.cvar_get_int: %s is not an int or does not exist, using fallback %d",
             name, fallback);
#endif
        return fallback;
    }
    return cv->value.i;
}

float cvar_get_float(cvar_table *t, const char *name, float fallback) {
    cvar *cv = cvar_get(t, name);
    if (!cv || cv->type != CVAR_FLOAT) {
#if CVAR_LOG_ENABLED
        log_debug(
             "cvar.cvar_get_float: %s is not a float or does not exist, using fallback "
             "%f",
             name, fallback);
#endif
        return fallback;
    }
    return cv->value.f;
}

bool cvar_get_bool(cvar_table *t, const char *name, bool fallback) {
    cvar *cv = cvar_get(t, name);
    if (!cv || cv->type != CVAR_BOOL) {
#if CVAR_LOG_ENABLED
        log_debug(
             "cvar.cvar_get_bool: %s is not a boolean or does not exist, using fallback "
             "%d",
             name, fallback);
#endif
        return fallback;
    }
    return cv->value.b;
}

const char *cvar_get_string(cvar_table *t, const char *name, const char *fallback) {
    cvar *cv = cvar_get(t, name);
    if (!cv || cv->type != CVAR_STRING) {
#if CVAR_LOG_ENABLED
        log_debug(
             "cvar.cvar_get_string: %s is not a string or does not exist, using "
             "fallback %s",
             name, fallback);
#endif
        return fallback;
    }
    return cv->value.str.data;
}

#if CVAR_LOG_ENABLED
static const char *cvar_type_to_string(cvar_type type) {
    switch (type) {
        case CVAR_INT:
            return "int";
        case CVAR_FLOAT:
            return "float";
        case CVAR_BOOL:
            return "bool";
        case CVAR_STRING:
            return "string";
        default:
            return "unknown";
    }
}
#endif

static bool cvar_set(cvar_table *table, const char *name, cvar_type type,
                     const void *value) {
    if (!table || !name || !value) return false;

    size_t name_len = strlen(name);
    if (name_len >= CVAR_NAME_MAX) {
#if CVAR_LOG_ENABLED
        log_error("cvar.cvar_set: name too long");
#endif
        return false;
    }

    cvar *cv     = cvar_get(table, name);
    bool  is_new = (cv == NULL);

    //
    // If the cvar does not exist (is_new == true), create it
    //
    {
        if (is_new) {
            if (table->size >= CVAR_TABLE_MAX_SIZE) {
#if CVAR_LOG_ENABLED
                log_error("cvar.cvar_set: table is full. Max size is %d",
                          CVAR_TABLE_MAX_SIZE);
#endif
                return false;
            }

            if (table->data == NULL) {
                table->capacity =
                     (table->capacity == 0) ? CVAR_DEFAULT_CAPACITY : table->capacity;

                table->data = malloc(table->capacity * sizeof(cvar));
                if (!table->data) return false;
            } else if (table->size >= table->capacity) {
                size_t new_cap =
                     (table->capacity == 0) ? CVAR_DEFAULT_CAPACITY : table->capacity * 2;

                cvar *new_data = realloc(table->data, new_cap * sizeof(cvar));
                if (!new_data) return false;

                table->data     = new_data;
                table->capacity = new_cap;
            }

            cv = &table->data[table->size];
            memset(cv, 0, sizeof(*cv));
        }
    }

    //
    // set the value for the cvar
    //
    {
        char *old_str = (!is_new && cv->type == CVAR_STRING) ? cv->value.str.data : NULL;

        cv->type = type;
        switch (type) {
            case CVAR_STRING: {
                size_t need = strlen((const char *)value) + 1;

                if (old_str && cv->value.str.cap >= need) {
                    // Reuse the existing buffer; cap stays, no realloc.
                    memcpy(old_str, value, need);
                    old_str = NULL;  // keep it, do not free below
                } else {
                    char *new_str = malloc(need);
                    if (!new_str) return false;

                    memcpy(new_str, value, need);

                    cv->value.str.data = new_str;
                    cv->value.str.cap  = need;
                }
                break;
            }
            case CVAR_INT:
                cv->value.i = *(const int *)value;
                break;

            case CVAR_FLOAT:
                cv->value.f = *(const float *)value;
                break;

            case CVAR_BOOL:
                cv->value.b = *(const bool *)value;
                break;
            default:
                return false;
        }
        if (old_str) free(old_str);
    }

    if (is_new) {
        memcpy(cv->name, name, name_len + 1);
        table->size++;
#if CVAR_LOG_ENABLED
        log_debug("cvar.cvar_set: registered new cvar %s of type %s", name,
                  cvar_type_to_string(type));
#endif
    }

    return true;
}

bool cvar_set_int(cvar_table *t, const char *name, int val) {
    const cvar_constraint *c = cvar_find_constraint(t, name);
    if (!cvar_range_ok(c, CVAR_INT, &val)) {
        log_error("cvar.cvar_set_int: '%s' value %d is out of range", name, val);
        return false;
    }
    return cvar_set(t, name, CVAR_INT, &val);
}

bool cvar_set_float(cvar_table *t, const char *name, float val) {
    const cvar_constraint *c = cvar_find_constraint(t, name);
    if (!cvar_range_ok(c, CVAR_FLOAT, &val)) {
        log_error("cvar.cvar_set_float: '%s' value %f is out of range", name,
                  (double)val);
        return false;
    }
    return cvar_set(t, name, CVAR_FLOAT, &val);
}

bool cvar_set_bool(cvar_table *t, const char *name, bool val) {
    return cvar_set(t, name, CVAR_BOOL, &val);
}

bool cvar_set_string(cvar_table *t, const char *name, const char *val) {
    return cvar_set(t, name, CVAR_STRING, val);
}

static void single_cvar_print(const cvar *cv) {
    switch (cv->type) {
        case CVAR_INT:
            printf("%s = %d\n", cv->name, cv->value.i);
            break;

        case CVAR_FLOAT:
            printf("%s = %f\n", cv->name, cv->value.f);
            break;

        case CVAR_BOOL:
            printf("%s = %s\n", cv->name, cv->value.b ? "true" : "false");
            break;

        case CVAR_STRING:
            printf("%s = \"%s\"\n", cv->name, cv->value.str.data);
            break;

        default:
            printf("%s = <unknown>\n", cv->name);
            break;
    }
}

void cvar_print(const cvar_table *table) {
    if (!table) return;
    for (size_t i = 0; i < table->size; i++) {
        single_cvar_print(&table->data[i]);
    }
}

#endif
#endif
