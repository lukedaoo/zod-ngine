#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/command.h>
#include "cmd_manager_internal.h"

command_execute_result sys_cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "help: list of available commands",
    };
}

#endif
