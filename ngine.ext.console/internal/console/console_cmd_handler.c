#ifdef ZOD_NGINE_IMPLEMENTATION
#include "console_internal.h"

#if ZOD_CONSOLE_ENABLED

#include <ngine.lib/command.h>
#include <ngine.lib/log.h>

command_execute_result console_cmd_priv_register_log_hook(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "log-hook-register: takes no arguments",
        };
    }
    log_unregister_hook(console_priv_log_hook);
    log_register_hook(console_priv_log_hook);
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "log-hook registered",
    };
}

command_execute_result console_cmd_priv_unregister_log_hook(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "log-hook-unregister: takes no arguments",
        };
    }
    log_unregister_hook(console_priv_log_hook);
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "log-hook unregistered",
    };
}

command_execute_result console_cmd_priv_clear_console(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "clear: takes no arguments",
        };
    }
    console_priv_clear();
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "console cleared",
    };
}

#endif

#endif
