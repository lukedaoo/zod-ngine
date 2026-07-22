#ifndef ZOD_CMD_MANAGER_H
#define ZOD_CMD_MANAGER_H

#include <ngine.lib/command.h>

typedef struct cmd_manager cmd_manager;

void cmd_manager_priv_init(cmd_manager *mgr);
void cmd_manager_priv_destroy(cmd_manager *mgr);
bool cmd_manager_priv_register(cmd_manager *mgr, command_group group, const char *name,
                          command_execute_result (*handler)(int argc, char **argv));
bool cmd_manager_priv_unregister(cmd_manager *mgr, command_group group, const char *name);

command_execute_result cmd_manager_priv_execute(cmd_manager *mgr, command_group group,
                                           const char *name, int argc, char **argv);

#endif
