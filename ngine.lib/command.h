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

typedef struct command       command;
typedef struct command_table command_table;

void command_table_init(command_table *table);
void command_table_init_with_capacity(command_table *table,
                                      const size_t   system_command_cap,
                                      const size_t   user_defined_command_cap);
// @info: reserve capacity for at least `additional` more system commands.
void command_table_reserve_system_commands(command_table *table, const size_t additional);
// @info: reserve capacity for at least `additional` more user-defined commands.
void command_table_reserve_user_defined_commands(command_table *table,
                                                 const size_t   additional);
bool command_table_register(command_table *table, command_group group, const char *name,
                            command_execute_result (*handler)(int argc, char **argv));
bool command_table_unregister(command_table *table, command_group group,
                              const char *name);
void command_table_destroy(command_table *table);

command *command_table_get(const command_table *table, const command_group group,
                           const char *name);
command_execute_result command_execute_by_name(const command_table *table,
                                               const command_group  group,
                                               const char *name, int argc, char **argv);
command_execute_result command_execute(command *command, int argc, char **argv);

#ifdef COMMAND_IMPLEMENTATION

#include "collections/array_list.h"

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

static command *command_table_get_by_name(const array_list *list, const char *name) {
    for (size_t i = 0; i < list->header.size; i++) {
        command *cmd = (command *)array_list_get(list, i);
        if (strcmp(cmd->name, name) == 0) return cmd;
    }
    return NULL;
}

bool command_table_register(command_table *table, command_group group, const char *name,
                            command_execute_result (*handler)(int argc, char **argv)) {
    if (!table || !name || !handler) return false;

    command *existing = command_table_get_by_name(group == COMMAND_GROUP_SYSTEM
                                                       ? &table->system_commands
                                                       : &table->user_defined_commands,
                                                  name);
    if (existing) {
#if COMMAND_LOG_ENABLED
        log_error("command.register: command %s already exists", name);
#endif
        return false;
    }

    command command = {0};
    strncpy(command.name, name, COMMAND_MAX_NAME_LEN);
    command.group   = group;
    command.handler = handler;
    if (group == COMMAND_GROUP_SYSTEM) {
        return array_list_append(&table->system_commands, &command);
    } else if (group == COMMAND_GROUP_USER_DEFINED) {
        return array_list_append(&table->user_defined_commands, &command);
    }
    return false;
}

command *command_table_get(const command_table *table, const command_group group,
                           const char *name) {
    if (!table || !name) return NULL;
    if (group == COMMAND_GROUP_SYSTEM) {
        return command_table_get_by_name(&table->system_commands, name);
    } else if (group == COMMAND_GROUP_USER_DEFINED) {
        return command_table_get_by_name(&table->user_defined_commands, name);
    }
    return NULL;
}

bool command_table_unregister(command_table *table, command_group group,
                              const char *name) {
    if (!table || !name) return false;
    if (group == COMMAND_GROUP_SYSTEM) {
        for (size_t i = 0; i < table->system_commands.header.size; i++) {
            command *cmd = (command *)array_list_get(&table->system_commands, i);
            if (strcmp(cmd->name, name) == 0)
                return array_list_remove(&table->system_commands, i);
        }
    } else if (group == COMMAND_GROUP_USER_DEFINED) {
        for (size_t i = 0; i < table->user_defined_commands.header.size; i++) {
            command *cmd = (command *)array_list_get(&table->user_defined_commands, i);
            if (strcmp(cmd->name, name) == 0)
                return array_list_remove(&table->user_defined_commands, i);
        }
    }
    return false;
}

command_execute_result command_execute_by_name(const command_table *table,
                                               const command_group  group,
                                               const char *name, int argc, char **argv) {
    command *cmd = command_table_get(table, group, name);
    if (!cmd) {
#if COMMAND_LOG_ENABLED
        log_error("command.execute_by_name: command %s does not exist", name);
#endif
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "command.execute_by_name: command does not exist"  //
        };
    }
    return cmd->handler(argc, argv);
}

command_execute_result command_execute(command *cmd, int argc, char **argv) {
    if (!cmd) {
#if COMMAND_LOG_ENABLED
        log_error("command.execute: command is NULL");
#endif
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "command.execute: command is NULL"  //
        };
    }
    return cmd->handler(argc, argv);
}

#endif
#endif
