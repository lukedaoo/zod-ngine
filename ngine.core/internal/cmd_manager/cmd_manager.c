#ifdef ZOD_NGINE_IMPLEMENTATION

#include <assert.h>
#include <stddef.h>

#include "../../cmd_manager.h"
#include "cmd_manager_internal.h"

void cmd_manager_priv_register_default_system_commands(cmd_manager *mgr) {
    static const struct {
        const char *name;
        command_execute_result (*handler)(int argc, char **argv);
    } defaults[] = {
         {"reload-config-file", sys_cmd_priv_reload_config_file},
         {"show-commands", sys_cmd_priv_show_commands},
         {"set-config", sys_cmd_priv_set_config},
         {"get-config", sys_cmd_priv_get_config},
         {"show-config", sys_cmd_priv_show_config},
         {"show-keybinding", sys_cmd_priv_show_keybinding},
         {"get-keybinding-by-action", sys_cmd_priv_get_keybinding_by_action},
         {"get-keybinding-by-key", sys_cmd_priv_get_keybinding_by_key},
         {"bind", sys_cmd_priv_bind},
         {"unbind", sys_cmd_priv_unbind},
    };

    for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); ++i) {
        const command_handle h = command_table_register(
             &mgr->table, COMMAND_GROUP_SYSTEM, defaults[i].name, defaults[i].handler);
        assert(h != COMMAND_HANDLE_INVALID && "system command registration failed");
        (void)h;
    }
}

void cmd_manager_priv_init(cmd_manager *mgr) {
    command_table_init(&mgr->table);
    cmd_manager_priv_register_default_system_commands(mgr);
}

void cmd_manager_priv_destroy(cmd_manager *mgr) { command_table_destroy(&mgr->table); }

command_handle cmd_manager_priv_register(cmd_manager *mgr, command_group group,
                                         const char *name,
                                         command_execute_result (*handler)(int    argc,
                                                                           char **argv)) {
    if (!mgr) return COMMAND_HANDLE_INVALID;
    return command_table_register(&mgr->table, group, name, handler);
}

bool cmd_manager_priv_unregister(cmd_manager *mgr, command_group group,
                                 const char *name) {
    return command_table_unregister(&mgr->table, group, name);
}

command_execute_result cmd_manager_priv_execute(cmd_manager *mgr, command_group group,
                                                const char *name, int argc, char **argv) {
    return command_execute_by_name(&mgr->table, group, name, argc, argv);
}

#endif
