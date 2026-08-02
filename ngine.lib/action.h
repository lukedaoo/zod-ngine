#ifndef ACTION_H
#define ACTION_H

typedef struct action_table          action_table;
typedef struct action                action;
typedef struct action_executor       action_executor;
typedef struct action_binding        action_binding;
typedef struct action_execute_result action_execute_result;
typedef int                          action_trigger_type;
typedef int                          action_mode;

typedef enum {
    ACTION_EXECUTE_RESULT_VOID,
    ACTION_EXECUTE_RESULT_INT,
    ACTION_EXECUTE_RESULT_FLOAT,
    ACTION_EXECUTE_RESULT_STRING,
    ACTION_EXECUTE_RESULT_PTR,
    ACTION_EXECUTE_RESULT_ERROR,
} action_execute_result_type;

void action_init(action_table *table);
void action_destroy(action_table *table);

action *action_resolve_by_name(const action_table *table, const action_mode context,
                               const action_trigger_type type, const char *name);
action *action_resolve_by_key(const action_table *table, const action_mode context,
                              const action_trigger_type type, const int key);

bool action_bind(action_table *table, const action_mode context,
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

action_execute_result action_execute(action *action, void *userdata);
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

struct action_binding {
    char name[ACTION_NAME_MAX];
    int  key;
};

struct action_execute_result {
    const action_execute_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
};

struct action_executor {
    action_execute_result (*execute)(const action *action, void *userdata);
};

struct action {
    action_executor     executor;
    action_binding      binding;
    action_mode         context;
    action_trigger_type type;
};

struct action_table {
    array_list actions;
};

void action_init(action_table *table) {
    if (!table) return;
    array_list_init(&table->actions, DEFAULT_ACTION_CAPACITY, sizeof(action));
}

void action_destroy(action_table *table) {
    if (!table) return;
    array_list_deinit(&table->actions);
}

action *action_resolve_by_name(const action_table *table, const action_mode context,
                               const action_trigger_type type, const char *name) {
    if (!table) return NULL;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            strcmp(entry->binding.name, name) == 0) {
            return entry;
        }
    }
    return NULL;
}

action *action_resolve_by_key(const action_table *table, const action_mode context,
                              const action_trigger_type type, const int key) {
    if (!table) return NULL;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            entry->binding.key == key) {
            return entry;
        }
    }
    return NULL;
}

bool action_bind(action_table *table, action_mode context, action_trigger_type type,
                 int key, const char *name, action_executor *executor) {
    if (!table || !executor || !name || strlen(name) >= ACTION_NAME_MAX) return false;

    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *a = (action *)array_list_get(&table->actions, i);
        if (a->context == context && a->type == type && a->binding.key == key) {
            return false;
        }
    }

    action new_action = {.context  = context,
                         .type     = type,
                         .binding  = {.key = key},
                         .executor = *executor};
    memcpy(new_action.binding.name, name, strlen(name) + 1);

    return array_list_append(&table->actions, &new_action);
}

bool action_rebind(action_table *table, const action_mode context,
                   const action_trigger_type type, const int old_key, const int new_key) {
    if (!table) return false;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            entry->binding.key == old_key) {
            entry->binding.key = new_key;
            return true;
        }
    }
    return false;
}

bool action_unbind_by_key(action_table *table, const action_mode context,
                          const action_trigger_type type, const int key) {
    if (!table) return false;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            entry->binding.key == key) {
            return array_list_remove(&table->actions, i);
        }
    }
    return false;
}

bool action_unbind_by_name(action_table *table, const action_mode context,
                           const action_trigger_type type, const char *name) {
    if (!table) return false;
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            strcmp(entry->binding.name, name) == 0) {
            return array_list_remove(&table->actions, i);
        }
    }
    return false;
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

action_execute_result action_execute(action *action, void *userdata) {
    if (!action) return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return action->executor.execute(action, userdata);
}

action_execute_result action_execute_by_name(action_table             *table,
                                             const action_mode         context,
                                             const action_trigger_type type,
                                             const char *name, void *userdata) {
    if (!table) return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    for (size_t i = 0; i < table->actions.header.size; i++) {
        action *entry = (action *)array_list_get(&table->actions, i);
        if (entry->context == context && entry->type == type &&
            strcmp(entry->binding.name, name) == 0) {
            return entry->executor.execute(entry, userdata);
        }
    }
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

#endif
#endif
