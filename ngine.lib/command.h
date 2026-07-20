#ifndef COMMAND_H
#define COMMAND_H

typedef enum { COMMAND_GROUP_SYSTEM, COMMAND_GROUP_USER_DEFINED } command_group;

// <command_name> <arg1> <arg2> ...
// flag must start with --
// <command_name> <flag> <arg1> <arg2> <flag> <arg1> <arg2> ...

typedef struct command                command;
typedef struct command_table          command_table;
typedef struct command_execute_result command_execute_result;

void command_table_init(command_table *table);
void command_table_init_with_capacity(command_table *table, const int system_command_cap,
                                      const int user_defined_command_cap);
// @info: reserve capacity for at least `additional` more system commands.
void command_table_reserve_system_commands(command_table *table, const int additional);
// @info: reserve capacity for at least `additional` more user-defined commands.
void command_table_reserve_user_defined_commands(command_table *table,
                                                 const int      additional);
bool command_table_register(command_table *table, const char *name, command_group group,
                            void (*handler)(int argc, char **argv));
bool command_table_unregister(command_table *table, const char *name);
void command_table_destroy(command_table *table);

command *command_table_get(const command_table *table, const char *name);
command *command_table_get_with_group(const command_table *table,
                                      const command_group group, const char *name);
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
    const char    name[COMMAND_MAX_NAME_LEN];
    command_group group;
    void (*handler)(int argc, char **argv);
};

struct command_table {
    array_list system_commands;        // of command
    array_list user_defined_commands;  // of command
};

void command_table_init(command_table *table) {
    command_table_init_with_capacity(table, COMMAND_TABLE_SYSTEM_COMMAND_INIT_SIZE,
                                     COMMAND_TABLE_USER_COMMAND_INITIAL_CAPACITY);
}

void command_table_init_with_capacity(command_table *table, const int system_command_cap,
                                      const int user_defined_command_cap) {
    if (!table) return;
    array_list_init(&table->system_commands, system_command_cap, sizeof(command));
    array_list_init(&table->user_defined_commands, user_defined_command_cap,
                    sizeof(command));
}

void command_table_destroy(command_table *table) {
    if (!table) return;
    // TODO: once command_table_register owns command.name (strdup), loop both
    // lists and free each cmd->name here before deinit, or these leak.
    array_list_deinit(&table->system_commands);
    array_list_deinit(&table->user_defined_commands);
}

void command_table_reserve_system_commands(command_table *table, const int additional) {
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
                                                 const int      additional) {
    if (!table || additional <= 0) {
#if COMMAND_LOG_ENABLED
        log_error("command.reserve_user_defined_commands: additional <= 0. Must > 0");
#endif
        return;
    }
    array_list_reserve(&table->user_defined_commands,
                       table->user_defined_commands.header.size + additional);
}

#endif
#endif
