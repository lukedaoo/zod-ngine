#ifndef CONTROL_H
#define CONTROL_H

#include <stddef.h>
#include <stdbool.h>

#include "action.h"
#include "log.h"

#ifndef CONTROL_LOG_ENABLED
#define CONTROL_LOG_ENABLED 0
#endif

#ifndef CONTROL_ACTION_MAX
#define CONTROL_ACTION_MAX 32
#endif

#ifndef DEFAULT_CONTROL_CAPACITY
#define DEFAULT_CONTROL_CAPACITY 16
#endif

#define CONTROL_HANDLE_INVALID 0u

typedef unsigned int control_handle;

typedef struct {
    action_mode         context;
    char                action[CONTROL_ACTION_MAX];
    int                 key;
    action_trigger_type trigger;
    bool                live;
} control_binding;

typedef struct control_table control_table;

typedef struct {
    const char *name;
    action_mode context;
} control_context;

typedef struct {
    const char         *name;
    action_trigger_type trigger;
} control_trigger_name;

typedef struct {
    const control_context *contexts;
    size_t                 context_count;

    const control_trigger_name *triggers;       // NULL selects the built-in table
    size_t                      trigger_count;  // ignored when triggers is NULL

    int (*key_from_name)(const char *name);  // returns 0 when unresolved
    const char *(*key_to_name)(int key);     // NULL disables control_key_to_string
} control_vocab;

bool   control_init(control_table *table);
void   control_deinit(control_table *table);
bool   control_clear(control_table *table);
size_t control_count(const control_table *table);

control_handle control_set(control_table *table, const action_mode context,
                           const char *action, const int key,
                           const action_trigger_type trigger);
bool           control_unset(control_table *table, const control_handle handle);

control_handle control_find(const control_table *table, const action_mode context,
                            const char *action);
control_handle control_find_by_key(const control_table *table, const action_mode context,
                                   const int key, const action_trigger_type trigger);
control_handle control_at(const control_table *table, const size_t index);
bool           control_lookup(const control_table *table, const control_handle handle,
                              control_binding *out);

bool                        control_vocab_valid(const control_vocab *vocab);
const control_trigger_name *control_trigger_table(const control_vocab *vocab,
                                                  size_t              *count);

const char *control_key_to_string(const control_vocab *vocab, const int key);
bool        control_trigger_from_string(const control_vocab *vocab, const char *name,
                                        action_trigger_type *out);
const char *control_trigger_to_string(const control_vocab      *vocab,
                                      const action_trigger_type trigger);

#ifdef CONTROL_IMPLEMENTATION

#include <string.h>

#include "collections/array_list.h"

#define CONTROL_INDEX_NONE ((size_t)-1)

struct control_table {
    array_list bindings;
};

static const control_trigger_name control_default_triggers[] = {
     {"pressed", 0},
     {"down", 1},
     {"released", 2},
};

static const size_t control_default_trigger_count =
     sizeof(control_default_triggers) / sizeof(control_default_triggers[0]);

const control_trigger_name *control_trigger_table(const control_vocab *vocab,
                                                  size_t              *count) {
    if (vocab && vocab->triggers && vocab->trigger_count > 0) {
        *count = vocab->trigger_count;
        return vocab->triggers;
    }
    *count = control_default_trigger_count;
    return control_default_triggers;
}

bool control_vocab_valid(const control_vocab *vocab) {
    return vocab && vocab->key_from_name && vocab->contexts && vocab->context_count > 0;
}

static control_handle control_handle_make(const size_t index) {
    return (control_handle)(index + 1u);
}

static control_binding *control_slot(const control_table *table, const size_t index) {
    if (!table) return NULL;
    return (control_binding *)array_list_get(&table->bindings, index);
}

static control_binding *control_resolve(const control_table *table,
                                        const control_handle handle) {
    if (!table || handle == CONTROL_HANDLE_INVALID) return NULL;
    return control_slot(table, (size_t)handle - 1u);
}

bool control_init(control_table *table) {
    if (!table) return false;
    return array_list_init(&table->bindings, DEFAULT_CONTROL_CAPACITY,
                           sizeof(control_binding));
}

void control_deinit(control_table *table) {
    if (!table) return;
    array_list_deinit(&table->bindings);
}

bool control_clear(control_table *table) {
    if (!table) return false;
    return array_list_clear(&table->bindings);
}

size_t control_count(const control_table *table) {
    if (!table) return 0;
    return table->bindings.header.size;
}

control_handle control_set(control_table *table, const action_mode context,
                           const char *action, const int key,
                           const action_trigger_type trigger) {
    if (!table || !action || key == 0) return CONTROL_HANDLE_INVALID;

    if (strlen(action) >= CONTROL_ACTION_MAX) {
#if CONTROL_LOG_ENABLED
        log_error("control.control_set: action name too long (max %d): '%s'",
                  CONTROL_ACTION_MAX - 1, action);
#endif
        return CONTROL_HANDLE_INVALID;
    }

    size_t same_action = CONTROL_INDEX_NONE;
    size_t conflict    = CONTROL_INDEX_NONE;
    size_t free_slot   = CONTROL_INDEX_NONE;

    for (size_t i = 0; i < table->bindings.header.size; i++) {
        control_binding *entry = control_slot(table, i);
        if (!entry) continue;

        if (!entry->live) {
            if (free_slot == CONTROL_INDEX_NONE) free_slot = i;
            continue;
        }
        if (entry->context != context) continue;

        if (strcmp(entry->action, action) == 0) {
            same_action = i;
        } else if (conflict == CONTROL_INDEX_NONE && entry->key == key &&
                   entry->trigger == trigger) {
            conflict = i;
        }
    }

    if (conflict != CONTROL_INDEX_NONE) {
        const control_binding *entry = control_slot(table, conflict);
#if CONTROL_LOG_ENABLED
        log_error(
             "control.control_set: context %d key %d already bound to '%s', "
             "refusing '%s'",
             context, key, entry->action, action);
#else
        (void)entry;
#endif
        return CONTROL_HANDLE_INVALID;
    }

    if (same_action != CONTROL_INDEX_NONE) {
        control_binding *entry = control_slot(table, same_action);
        entry->key             = key;
        entry->trigger         = trigger;
        return control_handle_make(same_action);
    }

    control_binding fresh = {
         .context = context,
         .key     = key,
         .trigger = trigger,
         .live    = true  //
    };
    memcpy(fresh.action, action, strlen(action) + 1);

    if (free_slot != CONTROL_INDEX_NONE) {
        *control_slot(table, free_slot) = fresh;
        return control_handle_make(free_slot);
    }

    if (!array_list_append(&table->bindings, &fresh)) {
#if CONTROL_LOG_ENABLED
        log_error("control.control_set: table full, dropping '%s'", action);
#endif
        return CONTROL_HANDLE_INVALID;
    }
    return control_handle_make(table->bindings.header.size - 1u);
}

bool control_unset(control_table *table, const control_handle handle) {
    control_binding *entry = control_resolve(table, handle);
    if (!entry || !entry->live) return false;

    entry->live      = false;
    entry->key       = 0;
    entry->action[0] = '\0';
    return true;
}

control_handle control_find(const control_table *table, const action_mode context,
                            const char *action) {
    if (!table || !action) return CONTROL_HANDLE_INVALID;

    for (size_t i = 0; i < table->bindings.header.size; i++) {
        const control_binding *entry = control_slot(table, i);
        if (!entry || !entry->live) continue;
        if (entry->context == context && strcmp(entry->action, action) == 0)
            return control_handle_make(i);
    }
    return CONTROL_HANDLE_INVALID;
}

control_handle control_find_by_key(const control_table *table, const action_mode context,
                                   const int key, const action_trigger_type trigger) {
    if (!table) return CONTROL_HANDLE_INVALID;

    for (size_t i = 0; i < table->bindings.header.size; i++) {
        const control_binding *entry = control_slot(table, i);
        if (!entry || !entry->live) continue;
        if (entry->context == context && entry->key == key && entry->trigger == trigger)
            return control_handle_make(i);
    }
    return CONTROL_HANDLE_INVALID;
}

control_handle control_at(const control_table *table, const size_t index) {
    const control_binding *entry = control_slot(table, index);
    if (!entry || !entry->live) return CONTROL_HANDLE_INVALID;
    return control_handle_make(index);
}

bool control_lookup(const control_table *table, const control_handle handle,
                    control_binding *out) {
    if (!out) return false;

    const control_binding *entry = control_resolve(table, handle);
    if (!entry || !entry->live) return false;

    *out = *entry;
    return true;
}

const char *control_key_to_string(const control_vocab *vocab, const int key) {
    if (!vocab || !vocab->key_to_name) return NULL;
    return vocab->key_to_name(key);
}

bool control_trigger_from_string(const control_vocab *vocab, const char *name,
                                 action_trigger_type *out) {
    if (!name || !out) return false;

    size_t                      count = 0;
    const control_trigger_name *table = control_trigger_table(vocab, &count);

    for (size_t i = 0; i < count; i++) {
        if (strcmp(table[i].name, name) == 0) {
            *out = table[i].trigger;
            return true;
        }
    }
    return false;
}

const char *control_trigger_to_string(const control_vocab      *vocab,
                                      const action_trigger_type trigger) {
    size_t                      count = 0;
    const control_trigger_name *table = control_trigger_table(vocab, &count);

    for (size_t i = 0; i < count; i++)
        if (table[i].trigger == trigger) return table[i].name;
    return NULL;
}

#endif
#endif
