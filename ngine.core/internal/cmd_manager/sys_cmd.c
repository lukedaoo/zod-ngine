#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/command.h>
#include "cmd_manager_internal.h"
#include "../../config.h"
#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

command_execute_result sys_cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "help: list of available commands",
    };
}

command_execute_result sys_cmd_reload_config(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "reload-config: takes no arguments",
        };
    }

    if (!config_reload_from_file(&g_ctx.config)) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "reload-config: failed to reload. See log for details",
        };
    }

    zod_ngine_apply_config(true);
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "config reloaded",
    };
}

#endif
