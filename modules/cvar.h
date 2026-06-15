#ifndef CVAR_H
#define CVAR_H

typedef enum { CVAR_INT, CVAR_FLOAT, CVAR_BOOL, CVAR_STRING } cvar_type;

typedef struct cvar_table cvar_table;
typedef struct cvar_t     cvar_t;

void cvar_destroy(cvar_table *table);

cvar_t *cvar_get(cvar_table *table, const char *name);

// Upserts a cvar into the table.
bool cvar_set(cvar_table *table, const char *name, cvar_type type, void *value);

#ifdef CVAR_IMPLEMENTATION

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

bool cvar_set(cvar_table *table, const char *name, cvar_type type, void *value) {
    if (!table) return false;
    if (!name) return false;

    if (strlen(name) >= CVAR_NAME_MAX) return false;

    cvar_t *cv     = cvar_get(table, name);
    bool    is_new = (cv == NULL);

    if (cv) {
        if (cv->type == CVAR_STRING) free(cv->value.s);
    } else {
        if (table->size >= CVAR_TABLE_MAX_SIZE) return false;

        if (table->data == NULL) {
            table->capacity =
                 (table->capacity == 0) ? CVAR_DEFAULT_CAPACITY : table->capacity;
            table->data = malloc(sizeof(cvar_t) * table->capacity);
            if (!table->data) return false;
        } else if (table->size >= table->capacity) {
            size_t  new_cap  = table->capacity * 2;
            cvar_t *new_data = realloc(table->data, sizeof(cvar_t) * new_cap);
            if (!new_data) return false;
            table->data     = new_data;
            table->capacity = new_cap;
        }

        cv = &table->data[table->size];
    }

    memcpy(cv->name, name, strlen(name) + 1);
    cv->type     = type;
    cv->capacity = 0;

    switch (type) {
        case CVAR_STRING:
            cv->capacity = strlen((char *)value) + 1;
            cv->value.s  = (char *)malloc(cv->capacity);
            if (!cv->value.s) return false;
            strcpy(cv->value.s, (char *)value);
            break;
        case CVAR_INT:
            cv->value.i = *(int *)value;
            break;
        case CVAR_FLOAT:
            cv->value.f = *(float *)value;
            break;
        case CVAR_BOOL:
            cv->value.b = *(bool *)value;
            break;
    }

    if (is_new) table->size++;
    return true;
}
#endif
#endif
