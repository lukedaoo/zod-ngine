#ifndef CMD_MANAGER_INTERNAL_H
#define CMD_MANAGER_INTERNAL_H

#include "../../cmd_manager.h"

struct cmd_manager {
    command_table table;
};

void cmd_manager_priv_register_default_system_commands(cmd_manager *mgr);
// --- System commands ---
command_execute_result sys_cmd_priv_reload_config_file(int argc, char **argv);
command_execute_result sys_cmd_priv_show_commands(int argc, char **argv);
command_execute_result sys_cmd_priv_set_config(int argc, char **argv);
command_execute_result sys_cmd_priv_get_config(int argc, char **argv);

#endif
