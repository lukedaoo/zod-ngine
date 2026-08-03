#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>
#include <stdbool.h>

typedef enum { COMMAND_GROUP_SYSTEM, COMMAND_GROUP_USER_DEFINED } command_group;

typedef enum {
    COMMAND_RESULT_VOID,
    COMMAND_RESULT_INT,
    COMMAND_RESULT_FLOAT,
    COMMAND_RESULT_STRING,
    COMMAND_RESULT_PTR,
    COMMAND_RESULT_ERROR,
    COMMAND_RESULT_COMMAND_NOT_FOUND,
} command_result_type;

typedef struct {
    command_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
} command_execute_result;

typedef struct command_table command_table;

// @info: opaque handle to a registered command. COMMAND_HANDLE_INVALID (0) is
// never a valid handle, so a zero-initialised handle is invalid by default.
//
// Lifetime: a handle is valid from the moment it is returned until any of the
// following occurs: the command it names is unregistered; *any other* command
// in the same table is unregistered; or the table is destroyed. Unregistering
// compacts the underlying storage and shifts the index of every later command,
// so all handles obtained before a removal must be treated as invalid
// afterwards — re-resolve with command_table_get. A handle is scoped to the
// table that produced it; passing it to a different table is undefined. The
// caller does not free handles; the table owns the storage.
typedef unsigned int command_handle;
#define COMMAND_HANDLE_INVALID 0u

void command_table_init(command_table *table);
void command_table_init_with_capacity(command_table *table,
                                      const size_t   system_command_cap,
                                      const size_t   user_defined_command_cap);
// @info: reserve capacity for at least `additional` more system commands.
void command_table_reserve_system_commands(command_table *table, const size_t additional);
// @info: reserve capacity for at least `additional` more user-defined commands.
void command_table_reserve_user_defined_commands(command_table *table,
                                                 const size_t   additional);
// @info: returns COMMAND_HANDLE_INVALID on bad arguments or duplicate name.
command_handle command_table_register(command_table *table, command_group group,
                                      const char *name,
                                      command_execute_result (*handler)(int    argc,
                                                                        char **argv));
bool           command_table_unregister(command_table *table, command_group group,
                                        const char *name);
void           command_table_destroy(command_table *table);

// @info: returns COMMAND_HANDLE_INVALID when no such command exists.
command_handle command_table_get(const command_table *table, const command_group group,
                                 const char *name);
command_execute_result command_execute_by_name(const command_table *table,
                                               const command_group  group,
                                               const char *name, int argc, char **argv);
command_execute_result command_execute(const command_table *table,
                                       const command_handle handle, int argc,
                                       char **argv);

#ifdef COMMAND_IMPLEMENTATION

#include "collections/array_list.h"

#include <string.h>

#ifndef COMMAND_LOG_ENABLED
#define COMMAND_LOG_ENABLED 0
#endif

#ifndef COMMAND_MAX_NAME_LEN
#define COMMAND_MAX_NAME_LEN 32
#endif

#ifndef COMMAND_MAX_ARGC
#define COMMAND_MAX_ARGC 32
#endif

#ifndef COMMAND_MAX_ARG_LEN
#define COMMAND_MAX_ARG_LEN 16
#endif

#ifndef COMMAND_TABLE_SYSTEM_COMMAND_INIT_SIZE
#define COMMAND_TABLE_SYSTEM_COMMAND_INIT_SIZE 16
#endif

#ifndef COMMAND_TABLE_SYSTEM_COMMAND_MAX_SIZE
#define COMMAND_TABLE_SYSTEM_COMMAND_MAX_SIZE 256
#endif

#ifndef COMMAND_TABLE_USER_COMMAND_INITIAL_CAPACITY
#define COMMAND_TABLE_USER_COMMAND_INITIAL_CAPACITY 16
#endif

struct command {
    char          name[COMMAND_MAX_NAME_LEN];
    command_group group;
    command_execute_result (*handler)(int argc, char **argv);
};

struct command_table {
    array_list system_commands;
    array_list user_defined_commands;
};

typedef struct command command;

// @info: handle layout — ((index + 1) << 1) | group. Group occupies the low
// bit so that handle 0 stays reserved for COMMAND_HANDLE_INVALID.
static command_handle command_handle_make(const size_t index, const command_group group) {
    return (command_handle)(((index + 1u) << 1u) | (unsigned int)group);
}

static command_group command_handle_group(const command_handle handle) {
    return (command_group)(handle & 1u);
}

static size_t command_handle_index(const command_handle handle) {
    return (size_t)(handle >> 1u) - 1u;
}

static const array_list *command_table_list(const command_table *table,
                                            const command_group  group) {
    return group == COMMAND_GROUP_SYSTEM ? &table->system_commands
                                         : &table->user_defined_commands;
}

static array_list *command_table_list_mut(command_table      *table,
                                          const command_group group) {
    return group == COMMAND_GROUP_SYSTEM ? &table->system_commands
                                         : &table->user_defined_commands;
}

static command *command_resolve(const command_table *table, const command_handle handle) {
    if (!table || handle == COMMAND_HANDLE_INVALID) return NULL;
    return (command *)array_list_get(
         command_table_list(table, command_handle_group(handle)),
         command_handle_index(handle));
}

void command_table_init(command_table *table) {
    command_table_init_with_capacity(table, COMMAND_TABLE_SYSTEM_COMMAND_INIT_SIZE,
                                     COMMAND_TABLE_USER_COMMAND_INITIAL_CAPACITY);
}

void command_table_init_with_capacity(command_table *table,
                                      const size_t   system_command_cap,
                                      const size_t   user_defined_command_cap) {
    if (!table) return;
    array_list_init(&table->system_commands, system_command_cap, sizeof(command));
    array_list_init(&table->user_defined_commands, user_defined_command_cap,
                    sizeof(command));
}

void command_table_destroy(command_table *table) {
    if (!table) return;
    array_list_deinit(&table->system_commands);
    array_list_deinit(&table->user_defined_commands);
}

void command_table_reserve_system_commands(command_table *table,
                                           const size_t   additional) {
    if (!table || additional <= 0) {
#if COMMAND_LOG_ENABLED
        log_error("command.reserve_system_defined_commands: additional <= 0. Must > 0");
#endif
        return;
    }
    array_list_reserve(&table->system_commands,
                       table->system_commands.header.size + additional);
}

void command_table_reserve_user_defined_commands(command_table *table,
                                                 const size_t   additional) {
    if (!table || additional <= 0) {
#if COMMAND_LOG_ENABLED
        log_error("command.reserve_user_defined_commands: additional <= 0. Must > 0");
#endif
        return;
    }
    array_list_reserve(&table->user_defined_commands,
                       table->user_defined_commands.header.size + additional);
}

#define COMMAND_INDEX_NONE ((size_t)-1)

static size_t command_index_by_name(const array_list *list, const char *name) {
    for (size_t i = 0; i < list->header.size; i++) {
        command *cmd = (command *)array_list_get(list, i);
        if (strcmp(cmd->name, name) == 0) return i;
    }
    return COMMAND_INDEX_NONE;
}

command_handle command_table_register(command_table *table, command_group group,
                                      const char *name,
                                      command_execute_result (*handler)(int    argc,
                                                                        char **argv)) {
    if (!table || !name || !handler) return COMMAND_HANDLE_INVALID;

    array_list *list = command_table_list_mut(table, group);
    if (command_index_by_name(list, name) != COMMAND_INDEX_NONE) {
#if COMMAND_LOG_ENABLED
        log_error("command.register: command %s already exists", name);
#endif
        return COMMAND_HANDLE_INVALID;
    }

    command command = {0};
    strncpy(command.name, name, COMMAND_MAX_NAME_LEN);
    command.group   = group;
    command.handler = handler;

    const size_t index = list->header.size;
    if (!array_list_append(list, &command)) return COMMAND_HANDLE_INVALID;

#if COMMAND_LOG_ENABLED
    log_info("command.register: command %s registered", name);
#endif
    return command_handle_make(index, group);
}

command_handle command_table_get(const command_table *table, const command_group group,
                                 const char *name) {
    if (!table || !name) return COMMAND_HANDLE_INVALID;
    const size_t index = command_index_by_name(command_table_list(table, group), name);
    if (index == COMMAND_INDEX_NONE) return COMMAND_HANDLE_INVALID;
    return command_handle_make(index, group);
}

bool command_table_unregister(command_table *table, command_group group,
                              const char *name) {
    if (!table || !name) return false;
    array_list  *list  = command_table_list_mut(table, group);
    const size_t index = command_index_by_name(list, name);
    if (index == COMMAND_INDEX_NONE) return false;
    return array_list_remove(list, index);
}

command_execute_result command_execute_by_name(const command_table *table,
                                               const command_group  group,
                                               const char *name, int argc, char **argv) {
    const command_handle handle = command_table_get(table, group, name);
    if (handle == COMMAND_HANDLE_INVALID) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_COMMAND_NOT_FOUND,
             .value.str = "command.execute_by_name: command does not exist"  //
        };
    }
    return command_execute(table, handle, argc, argv);
}

command_execute_result command_execute(const command_table *table,
                                       const command_handle handle, int argc,
                                       char **argv) {
    command *cmd = command_resolve(table, handle);
    if (!cmd) {
#if COMMAND_LOG_ENABLED
        log_error("command.execute: invalid handle %u", handle);
#endif
        return (command_execute_result){
             .type      = COMMAND_RESULT_COMMAND_NOT_FOUND,
             .value.str = "command.execute: command does not exist"  //
        };
    }
    return cmd->handler(argc, argv);
}

#endif
#endif
