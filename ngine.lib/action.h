#ifndef ACTION_H
#define ACTION_H

#include <stddef.h>
#include <stdbool.h>

typedef struct action_table action_table;
typedef int                 action_trigger_type;
typedef int                 action_mode;

typedef enum {
    ACTION_EXECUTE_RESULT_VOID,
    ACTION_EXECUTE_RESULT_INT,
    ACTION_EXECUTE_RESULT_FLOAT,
    ACTION_EXECUTE_RESULT_STRING,
    ACTION_EXECUTE_RESULT_PTR,
    ACTION_EXECUTE_RESULT_ERROR,
} action_execute_result_type;

typedef struct {
    action_execute_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
} action_execute_result;

// @info: opaque handle to a bound action. ACTION_HANDLE_INVALID (0) is never a
// valid handle, so a zero-initialised handle is invalid by default.
//
// Lifetime: a handle is valid from the moment it is returned until any of the
// following occurs: the action it names is unbound; *any other* action in the
// same table is unbound; or the table is destroyed. Unbinding compacts the
// underlying storage and shifts the index of every later action, so all
// handles obtained before an unbind must be treated as invalid afterwards —
// re-resolve with action_resolve_by_name or action_resolve_by_key. A handle is
// scoped to the table that produced it; passing it to a different table is
// undefined. The caller does not free handles; the table owns the storage.
typedef unsigned int action_handle;
#define ACTION_HANDLE_INVALID 0u

typedef action_execute_result (*action_exec_fn)(const action_table *table,
                                                action_handle self, void *userdata);

typedef struct {
    action_exec_fn execute;
} action_executor;

void action_init(action_table *table);
void action_destroy(action_table *table);

// @info: both return ACTION_HANDLE_INVALID when no such action is bound.
action_handle action_resolve_by_name(const action_table *table, const action_mode context,
                                     const action_trigger_type type, const char *name);
action_handle action_resolve_by_key(const action_table *table, const action_mode context,
                                    const action_trigger_type type, const int key);

// @info: returns ACTION_HANDLE_INVALID on bad arguments, or when the
// (context, type, key) triple is already bound.
action_handle action_bind(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key, const char *name,
                          action_executor *executor);

bool action_rebind(action_table *table, const action_mode context,
                   const action_trigger_type type, const int old_key, const int new_key);

bool action_unbind_by_key(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key);

bool action_unbind_by_name(action_table *table, const action_mode context,
                           const action_trigger_type type, const char *name);

bool action_unbind_all(action_table *table, const action_mode context,
                       const action_trigger_type type);

action_execute_result action_execute(const action_table *table,
                                     const action_handle handle, void *userdata);
action_execute_result action_execute_by_name(action_table             *table,
                                             const action_mode         context,
                                             const action_trigger_type type,
                                             const char *name, void *userdata);

#ifdef ACTION_IMPLEMENTATION

#include <string.h>

#include "collections/array_list.h"

#ifndef ACTION_LOG_ENABLED
#define ACTION_LOG_ENABLED 0
#endif

#ifndef DEFAULT_ACTION_CAPACITY
#define DEFAULT_ACTION_CAPACITY 16
#endif

#ifndef ACTION_NAME_MAX
#define ACTION_NAME_MAX 32
#endif

typedef struct action action;

typedef struct {
    char name[ACTION_NAME_MAX];
    int  key;
} action_binding;

struct action {
    action_executor     executor;
    action_binding      binding;
    action_mode         context;
    action_trigger_type type;
};

struct action_table {
    array_list actions;
};

#define ACTION_INDEX_NONE ((size_t)-1)

static action_handle action_handle_make(const size_t index) {
    return (action_handle)(index + 1u);
}

static action *action_resolve(const action_table *table, const action_handle handle) {
    if (!table || handle == ACTION_HANDLE_INVALID) return NULL;
    return (action *)array_list_get(&table->actions, (size_t)handle - 1u);
}

static size_t action_index_by_name(const action_table *table, const action_mode context,
                                   const action_trigger_type type, const char *name) {
    if (!table || !name) return ACTION_INDEX_NONE;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            strcmp(entry->binding.name, name) == 0) {
            return i;
        }
    }
    return ACTION_INDEX_NONE;
}

static size_t action_index_by_key(const action_table *table, const action_mode context,
                                  const action_trigger_type type, const int key) {
    if (!table) return ACTION_INDEX_NONE;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            entry->binding.key == key) {
            return i;
        }
    }
    return ACTION_INDEX_NONE;
}

void action_init(action_table *table) {
    if (!table) return;
    array_list_init(&table->actions, DEFAULT_ACTION_CAPACITY, sizeof(action));
}

void action_destroy(action_table *table) {
    if (!table) return;
    array_list_deinit(&table->actions);
}

action_handle action_resolve_by_name(const action_table *table, const action_mode context,
                                     const action_trigger_type type, const char *name) {
    const size_t index = action_index_by_name(table, context, type, name);
    if (index == ACTION_INDEX_NONE) return ACTION_HANDLE_INVALID;
    return action_handle_make(index);
}

action_handle action_resolve_by_key(const action_table *table, const action_mode context,
                                    const action_trigger_type type, const int key) {
    const size_t index = action_index_by_key(table, context, type, key);
    if (index == ACTION_INDEX_NONE) return ACTION_HANDLE_INVALID;
    return action_handle_make(index);
}

action_handle action_bind(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key, const char *name,
                          action_executor *executor) {
    if (!table || !executor || !name || strlen(name) >= ACTION_NAME_MAX)
        return ACTION_HANDLE_INVALID;

    if (action_index_by_key(table, context, type, key) != ACTION_INDEX_NONE)
        return ACTION_HANDLE_INVALID;

    action new_action = {.context  = context,
                         .type     = type,
                         .binding  = {.key = key},
                         .executor = *executor};
    memcpy(new_action.binding.name, name, strlen(name) + 1);

    const size_t index = table->actions.header.size;
    if (!array_list_append(&table->actions, &new_action)) return ACTION_HANDLE_INVALID;
    return action_handle_make(index);
}

bool action_rebind(action_table *table, const action_mode context,
                   const action_trigger_type type, const int old_key, const int new_key) {
    const size_t index = action_index_by_key(table, context, type, old_key);
    if (index == ACTION_INDEX_NONE) return false;
    action *entry      = (action *)array_list_get(&table->actions, index);
    entry->binding.key = new_key;
    return true;
}

bool action_unbind_by_key(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key) {
    const size_t index = action_index_by_key(table, context, type, key);
    if (index == ACTION_INDEX_NONE) return false;
    return array_list_remove(&table->actions, index);
}

bool action_unbind_by_name(action_table *table, const action_mode context,
                           const action_trigger_type type, const char *name) {
    const size_t index = action_index_by_name(table, context, type, name);
    if (index == ACTION_INDEX_NONE) return false;
    return array_list_remove(&table->actions, index);
}

bool action_unbind_all(action_table *table, const action_mode context,
                       const action_trigger_type type) {
    if (!table) return false;
    bool removed = false;
    for (size_t i = table->actions.header.size; i-- > 0;) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type) {
            array_list_remove(&table->actions, i);
            removed = true;
        }
    }
    return removed;
}

action_execute_result action_execute(const action_table *table,
                                     const action_handle handle, void *userdata) {
    action *entry = action_resolve(table, handle);
    if (!entry) return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return entry->executor.execute(table, handle, userdata);
}

action_execute_result action_execute_by_name(action_table             *table,
                                             const action_mode         context,
                                             const action_trigger_type type,
                                             const char *name, void *userdata) {
    const action_handle handle = action_resolve_by_name(table, context, type, name);
    if (handle == ACTION_HANDLE_INVALID)
        return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return action_execute(table, handle, userdata);
}

#endif
#endif
