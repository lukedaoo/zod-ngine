#ifndef KEYBIND_H
#define KEYBIND_H

#include <stddef.h>
#include <stdbool.h>

#include "log.h"

#ifndef KEYBIND_LOG_ENABLED
#define KEYBIND_LOG_ENABLED 0
#endif

#ifndef KEYBIND_ACTION_MAX
#define KEYBIND_ACTION_MAX 32
#endif

#ifndef DEFAULT_KEYBIND_CAPACITY
#define DEFAULT_KEYBIND_CAPACITY 16
#endif

#define KEYBIND_HANDLE_INVALID 0u

typedef unsigned int keybind_handle;

typedef struct {
    int         context;
    int                 key;
    int trigger;
    char                action[KEYBIND_ACTION_MAX];
    bool                live;
} keybind;

typedef struct keybind_table keybind_table;

typedef struct {
    const char *name;
    int context;
} keybind_context;

typedef struct {
    const char         *name;
    int trigger;
} keybind_trigger_name;

typedef struct {
    const keybind_context *contexts;
    size_t                 context_count;

    const keybind_trigger_name *triggers;
    size_t                      trigger_count;

    int (*key_from_name)(const char *name);
    const char *(*key_to_name)(int key);
} keybind_vocab;

bool   keybind_init(keybind_table *table);
void   keybind_deinit(keybind_table *table);
bool   keybind_clear(keybind_table *table);
size_t keybind_count(const keybind_table *table);

keybind_handle keybind_set(keybind_table *table, const int context, const int key,
                           const int trigger, const char *action);
bool           keybind_unset(keybind_table *table, const keybind_handle handle);

keybind_handle keybind_find_by_key(const keybind_table *table, const int context,
                                   const int key, const int trigger);
keybind_handle keybind_find_action(const keybind_table *table, const int context,
                                   const char *action);
keybind_handle keybind_at(const keybind_table *table, const size_t index);
bool           keybind_lookup(const keybind_table *table, const keybind_handle handle,
                              keybind *out);

bool                        keybind_vocab_valid(const keybind_vocab *vocab);
const keybind_trigger_name *keybind_trigger_table(const keybind_vocab *vocab,
                                                  size_t              *count);

const char *keybind_key_to_string(const keybind_vocab *vocab, const int key);
bool        keybind_trigger_from_string(const keybind_vocab *vocab, const char *name,
                                        int *out);
const char *keybind_trigger_to_string(const keybind_vocab      *vocab,
                                      const int trigger);

#ifdef KEYBIND_IMPLEMENTATION

#include <string.h>

#include "collections/array_list.h"

#define KEYBIND_INDEX_NONE ((size_t)-1)

struct keybind_table {
    array_list bindings;
};

static const keybind_trigger_name keybind_default_triggers[] = {
     {"pressed", 0},
     {"down", 1},
     {"released", 2},
};

static const size_t keybind_default_trigger_count =
     sizeof(keybind_default_triggers) / sizeof(keybind_default_triggers[0]);

const keybind_trigger_name *keybind_trigger_table(const keybind_vocab *vocab,
                                                  size_t              *count) {
    if (vocab && vocab->triggers && vocab->trigger_count > 0) {
        *count = vocab->trigger_count;
        return vocab->triggers;
    }
    *count = keybind_default_trigger_count;
    return keybind_default_triggers;
}

bool keybind_vocab_valid(const keybind_vocab *vocab) {
    return vocab && vocab->key_from_name && vocab->contexts && vocab->context_count > 0;
}

static keybind_handle keybind_handle_make(const size_t index) {
    return (keybind_handle)(index + 1u);
}

static keybind *keybind_slot(const keybind_table *table, const size_t index) {
    if (!table) return NULL;
    return (keybind *)array_list_get(&table->bindings, index);
}

static keybind *keybind_resolve(const keybind_table *table, const keybind_handle handle) {
    if (!table || handle == KEYBIND_HANDLE_INVALID) return NULL;
    return keybind_slot(table, (size_t)handle - 1u);
}

bool keybind_init(keybind_table *table) {
    if (!table) return false;
    return array_list_init(&table->bindings, DEFAULT_KEYBIND_CAPACITY, sizeof(keybind));
}

void keybind_deinit(keybind_table *table) {
    if (!table) return;
    array_list_deinit(&table->bindings);
}

bool keybind_clear(keybind_table *table) {
    if (!table) return false;
    return array_list_clear(&table->bindings);
}

size_t keybind_count(const keybind_table *table) {
    if (!table) return 0;
    return table->bindings.header.size;
}

keybind_handle keybind_set(keybind_table *table, const int context, const int key,
                           const int trigger, const char *action) {
    if (!table || !action || key == 0) return KEYBIND_HANDLE_INVALID;

    if (strlen(action) >= KEYBIND_ACTION_MAX) {
#if KEYBIND_LOG_ENABLED
        log_error("keybind.keybind_set: action name too long (max %d): '%s'",
                  KEYBIND_ACTION_MAX - 1, action);
#endif
        return KEYBIND_HANDLE_INVALID;
    }

    size_t same_key   = KEYBIND_INDEX_NONE;
    size_t free_slot  = KEYBIND_INDEX_NONE;

    for (size_t i = 0; i < table->bindings.header.size; i++) {
        keybind *entry = keybind_slot(table, i);
        if (!entry) continue;

        if (!entry->live) {
            if (free_slot == KEYBIND_INDEX_NONE) free_slot = i;
            continue;
        }
        if (entry->context == context && entry->key == key && entry->trigger == trigger) {
            same_key = i;
            break;
        }
    }

    if (same_key != KEYBIND_INDEX_NONE) {
        keybind *entry = keybind_slot(table, same_key);
        memcpy(entry->action, action, strlen(action) + 1);
        return keybind_handle_make(same_key);
    }

    keybind fresh = {
         .context = context,
         .key     = key,
         .trigger = trigger,
         .live    = true  //
    };
    memcpy(fresh.action, action, strlen(action) + 1);

    if (free_slot != KEYBIND_INDEX_NONE) {
        *keybind_slot(table, free_slot) = fresh;
        return keybind_handle_make(free_slot);
    }

    if (!array_list_append(&table->bindings, &fresh)) {
#if KEYBIND_LOG_ENABLED
        log_error("keybind.keybind_set: table full, dropping '%s'", action);
#endif
        return KEYBIND_HANDLE_INVALID;
    }
    return keybind_handle_make(table->bindings.header.size - 1u);
}

bool keybind_unset(keybind_table *table, const keybind_handle handle) {
    keybind *entry = keybind_resolve(table, handle);
    if (!entry || !entry->live) return false;

    entry->live      = false;
    entry->key       = 0;
    entry->action[0] = '\0';
    return true;
}

keybind_handle keybind_find_by_key(const keybind_table *table, const int context,
                                   const int key, const int trigger) {
    if (!table) return KEYBIND_HANDLE_INVALID;

    for (size_t i = 0; i < table->bindings.header.size; i++) {
        const keybind *entry = keybind_slot(table, i);
        if (!entry || !entry->live) continue;
        if (entry->context == context && entry->key == key && entry->trigger == trigger)
            return keybind_handle_make(i);
    }
    return KEYBIND_HANDLE_INVALID;
}

keybind_handle keybind_find_action(const keybind_table *table, const int context,
                                   const char *action) {
    if (!table || !action) return KEYBIND_HANDLE_INVALID;

    for (size_t i = 0; i < table->bindings.header.size; i++) {
        const keybind *entry = keybind_slot(table, i);
        if (!entry || !entry->live) continue;
        if (entry->context == context && strcmp(entry->action, action) == 0)
            return keybind_handle_make(i);
    }
    return KEYBIND_HANDLE_INVALID;
}

keybind_handle keybind_at(const keybind_table *table, const size_t index) {
    const keybind *entry = keybind_slot(table, index);
    if (!entry || !entry->live) return KEYBIND_HANDLE_INVALID;
    return keybind_handle_make(index);
}

bool keybind_lookup(const keybind_table *table, const keybind_handle handle,
                    keybind *out) {
    if (!out) return false;

    const keybind *entry = keybind_resolve(table, handle);
    if (!entry || !entry->live) return false;

    *out = *entry;
    return true;
}

const char *keybind_key_to_string(const keybind_vocab *vocab, const int key) {
    if (!vocab || !vocab->key_to_name) return NULL;
    return vocab->key_to_name(key);
}

bool keybind_trigger_from_string(const keybind_vocab *vocab, const char *name,
                                 int *out) {
    if (!name || !out) return false;

    size_t                      count = 0;
    const keybind_trigger_name *table = keybind_trigger_table(vocab, &count);

    for (size_t i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0) {
            *out = table[i].trigger;
            return true;
        }
    }
    return false;
}

const char *keybind_trigger_to_string(const keybind_vocab      *vocab,
                                      const int trigger) {
    size_t                      count = 0;
    const keybind_trigger_name *table = keybind_trigger_table(vocab, &count);

    for (size_t i = 0; i < count; i++)
        if (table[i].trigger == trigger) return table[i].name;
    return NULL;
}

#endif  // KEYBIND_IMPLEMENTATION
#endif  // KEYBIND_H
