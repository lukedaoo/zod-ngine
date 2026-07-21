#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../cmd_manager.h"
#include "cmd_manager_internal.h"

void cmd_manager_register_default_system_commands(cmd_manager *mgr) {
    command_table_register(&mgr->table, COMMAND_GROUP_SYSTEM, "help", sys_cmd_help);
}

void cmd_manager_init(cmd_manager *mgr) {
    command_table_init(&mgr->table);
    cmd_manager_register_default_system_commands(mgr);
}

void cmd_manager_destroy(cmd_manager *mgr) { command_table_destroy(&mgr->table); }

bool cmd_manager_register(cmd_manager *mgr, command_group group, const char *name,
                          command_execute_result (*handler)(int argc, char **argv)) {
    return command_table_register(&mgr->table, group, name, handler);
}

bool cmd_manager_unregister(cmd_manager *mgr, command_group group, const char *name) {
    return command_table_unregister(&mgr->table, group, name);
}

command_execute_result cmd_manager_execute(cmd_manager *mgr, command_group group,
                                           const char *name, int argc, char **argv) {
    return command_execute_by_name(&mgr->table, group, name, argc, argv);
}

#endif
