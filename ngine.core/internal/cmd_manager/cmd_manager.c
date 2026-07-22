#ifdef ZOD_NGINE_IMPLEMENTATION

#include <assert.h>

#include "../../cmd_manager.h"
#include "cmd_manager_internal.h"

void cmd_manager_priv_register_default_system_commands(cmd_manager *mgr) {
    command_table_register(&mgr->table, COMMAND_GROUP_SYSTEM, "help", sys_cmd_priv_help);
    command_table_register(&mgr->table, COMMAND_GROUP_SYSTEM, "reload-config",
                           sys_cmd_priv_reload_config);

    assert(mgr->table.system_commands.header.size == 2 && "expected 2 system commands");
}

void cmd_manager_priv_init(cmd_manager *mgr) {
    command_table_init(&mgr->table);
    cmd_manager_priv_register_default_system_commands(mgr);
}

void cmd_manager_priv_destroy(cmd_manager *mgr) { command_table_destroy(&mgr->table); }

bool cmd_manager_priv_register(cmd_manager *mgr, command_group group, const char *name,
                          command_execute_result (*handler)(int argc, char **argv)) {
    return command_table_register(&mgr->table, group, name, handler);
}

bool cmd_manager_priv_unregister(cmd_manager *mgr, command_group group, const char *name) {
    return command_table_unregister(&mgr->table, group, name);
}

command_execute_result cmd_manager_priv_execute(cmd_manager *mgr, command_group group,
                                           const char *name, int argc, char **argv) {
    return command_execute_by_name(&mgr->table, group, name, argc, argv);
}

#endif
