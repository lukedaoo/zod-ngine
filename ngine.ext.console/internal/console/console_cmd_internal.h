#ifndef CONSOLE_CMD_INTERNAL_H
#define CONSOLE_CMD_INTERNAL_H

#include <ngine.lib/command.h>

void console_cmd_priv_register(void);

command_execute_result console_cmd_priv_register_log_hook(int argc, char **argv);
command_execute_result console_cmd_priv_unregister_log_hook(int argc, char **argv);

command_execute_result console_cmd_priv_clear_console(int argc, char **argv);

#endif
