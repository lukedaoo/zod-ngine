#ifndef CONSOLE_CMD_INTERNAL_H
#define CONSOLE_CMD_INTERNAL_H

#include <ngine.lib/command.h>

command_execute_result console_cmd_log_hook_register(int argc, char **argv);
command_execute_result console_cmd_log_hook_unregister(int argc, char **argv);

command_execute_result console_cmd_clear_console(int argc, char **argv);

#endif
