#ifndef CVAR_H
#define CVAR_H

typedef enum { CVAR_INT, CVAR_FLOAT, CVAR_BOOL, CVAR_STRING } cvar_type;

typedef struct cvar_table cvar_table;
typedef struct cvar_t     cvar_t;

void cvar_destroy(cvar_table *table);

cvar_t *cvar_get(cvar_table *table, const char *name);

int         cvar_get_int(cvar_table *table, const char *name);
float       cvar_get_float(cvar_table *table, const char *name);
bool        cvar_get_bool(cvar_table *table, const char *name);
const char *cvar_get_string(cvar_table *table, const char *name);

bool cvar_set_int(cvar_table *t, const char *name, int val);
bool cvar_set_float(cvar_table *t, const char *name, float val);
bool cvar_set_bool(cvar_table *t, const char *name, bool val);
bool cvar_set_string(cvar_table *t, const char *name, const char *val);

#ifdef CVAR_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// @info: maximum size of cvar table
// @todo: make this configurable
#ifndef CVAR_TABLE_MAX_SIZE
#define CVAR_TABLE_MAX_SIZE 1024
#endif

#ifndef CVAR_DEFAULT_CAPACITY
#define CVAR_DEFAULT_CAPACITY 8
#endif

#ifndef CVAR_NAME_MAX
#define CVAR_NAME_MAX 64
#endif

struct cvar_t {
    char      name[CVAR_NAME_MAX];
    cvar_type type;
    size_t    capacity;
    union {
        int   i;
        float f;
        bool  b;
        char *s;
    } value;
};

struct cvar_table {
    cvar_t *data;
    size_t  size;
    size_t  capacity;
};

void cvar_destroy(cvar_table *table) {
    if (!table) return;
    for (size_t i = 0; i < table->size; i++) {
        if (table->data[i].type == CVAR_STRING) free(table->data[i].value.s);
    }
    free(table->data);
    table->data     = NULL;
    table->size     = 0;
    table->capacity = 0;
}

cvar_t *cvar_get(cvar_table *table, const char *name) {
    if (!table || !table->data || !name) return NULL;
    for (size_t i = 0; i < table->size; i++) {
        if (strcmp(table->data[i].name, name) == 0) return &table->data[i];
    }
    return NULL;
}

int cvar_get_int(cvar_table *table, const char *name) {
    cvar_t *cv = cvar_get(table, name);
    if (!cv) {
        fprintf(stderr,
                "cvar.cvar_get_int: the cvar %s does not exist. Return %d as default\n",
                name, 0);
        return 0;
    }
    if (cv->type != CVAR_INT) {
        fprintf(stderr,
                "cvar.cvar_get_int: tried to get %s as int, but it has type %d. Return "
                "%d as "
                "default\n",
                name, cv->type, 0);
        return 0;
    }
    return cv->value.i;
}

float cvar_get_float(cvar_table *table, const char *name) {
    cvar_t *cv = cvar_get(table, name);
    if (!cv) {
        fprintf(stderr,
                "cvar.cvar_get_float: the cvar %s does not exist. Return %f as default\n",
                name, 0.0F);
        return 0.0F;
    }
    if (cv->type != CVAR_FLOAT) {
        fprintf(stderr,
                "cvar.cvar_get_float: tried to get %s as float, but it has type %d. "
                "Return %f as "
                "default\n",
                name, cv->type, 0.0F);
        return 0.0F;
    }
    return cv->value.f;
}

bool cvar_get_bool(cvar_table *table, const char *name) {
    cvar_t *cv = cvar_get(table, name);
    if (!cv) {
        fprintf(stderr,
                "cvar.cvar_get_bool: the cvar %s does not exist. Return %d as default\n",
                name, false);
        return false;
    }
    if (cv->type != CVAR_BOOL) {
        fprintf(stderr,
                "cvar.cvar_get_bool: the cvar %s has type %d. Return %d as default\n",
                name, cv->type, false);
        return false;
    }
    return cv->value.b;
}

const char *cvar_get_string(cvar_table *table, const char *name) {
    cvar_t *cv = cvar_get(table, name);
    if (!cv) {
        fprintf(
             stderr,
             "cvar.cvar_get_string: the cvar %s does not exist. Return %s as default\n",
             name, "NULL");
        return NULL;
    }
    if (cv->type != CVAR_STRING) {
        fprintf(stderr,
                "cvar.cvar_get_string: the cvar %s has type %d. Return %s as default\n",
                name, cv->type, "NULL");
        return NULL;
    }
    return cv->value.s;
}

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

static bool cvar_set(cvar_table *table,
                     const char *name,
                     cvar_type   type,
                     const void *value) {
    if (!table || !name || !value) return false;

    size_t name_len = strlen(name);
    if (name_len >= CVAR_NAME_MAX) return false;

    cvar_t *cv     = cvar_get(table, name);
    bool    is_new = (cv == NULL);

    if (is_new) {
        if (table->size >= CVAR_TABLE_MAX_SIZE) return false;

        if (table->data == NULL) {
            table->capacity =
                 (table->capacity == 0) ? CVAR_DEFAULT_CAPACITY : table->capacity;

            table->data = malloc(table->capacity * sizeof(cvar_t));
            if (!table->data) return false;
        } else if (table->size >= table->capacity) {
            size_t new_cap =
                 (table->capacity == 0) ? CVAR_DEFAULT_CAPACITY : table->capacity * 2;

            cvar_t *new_data = realloc(table->data, new_cap * sizeof(cvar_t));
            if (!new_data) return false;

            table->data     = new_data;
            table->capacity = new_cap;
        }

        cv = &table->data[table->size];
        memset(cv, 0, sizeof(*cv));
    }

    char *old_str = (cv->type == CVAR_STRING) ? cv->value.s : NULL;

    cv->type = type;
    switch (type) {
        case CVAR_STRING:
            char  *new_str      = NULL;
            size_t str_capacity = 0;
            str_capacity        = strlen((const char *)value) + 1;

            new_str = malloc(str_capacity);
            if (!new_str) return false;

            memcpy(new_str, value, str_capacity);

            cv->value.s  = new_str;
            cv->capacity = str_capacity;
            break;
        case CVAR_INT:
            cv->value.i  = *(const int *)value;
            cv->capacity = 0;
            break;

        case CVAR_FLOAT:
            cv->value.f  = *(const float *)value;
            cv->capacity = 0;
            break;

        case CVAR_BOOL:
            cv->value.b  = *(const bool *)value;
            cv->capacity = 0;
            break;
        default:
            return false;
    }

    if (old_str) free(old_str);

    if (is_new) {
        memcpy(cv->name, name, name_len + 1);
        table->size++;

        printf("cvar.cvar_set: registered new cvar %s of type %s\n", name,
               cvar_type_to_string(type));
    }

    return true;
}

bool cvar_set_int(cvar_table *t, const char *name, int val) {
    return cvar_set(t, name, CVAR_INT, &val);
}
bool cvar_set_float(cvar_table *t, const char *name, float val) {
    return cvar_set(t, name, CVAR_FLOAT, &val);
}
bool cvar_set_bool(cvar_table *t, const char *name, bool val) {
    return cvar_set(t, name, CVAR_BOOL, &val);
}
bool cvar_set_string(cvar_table *t, const char *name, const char *val) {
    return cvar_set(t, name, CVAR_STRING, val);
}
#endif
#endif
