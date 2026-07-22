#ifdef ZOD_NGINE_IMPLEMENTATION
#include <ngine.lib/command.h>
#include <ngine.lib/log.h>
#include "console_internal.h"

command_execute_result console_cmd_log_hook_register(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "log-hook-register: takes no arguments",
        };
    }
    log_unregister_hook(console_log_hook);
    log_register_hook(console_log_hook);
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "log-hook registered",
    };
}

command_execute_result console_cmd_log_hook_unregister(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "log-hook-unregister: takes no arguments",
        };
    }
    log_unregister_hook(console_log_hook);
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "log-hook unregistered",
    };
}

command_execute_result console_cmd_clear_console(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "clear-console: takes no arguments",
        };
    }
    console_clear();
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "console cleared",
    };
}

#endif
