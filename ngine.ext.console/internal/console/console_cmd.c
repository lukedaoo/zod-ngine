#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/command.h>
#include <ngine.core/zod_ngine.h>

#include "console_internal.h"
#include "console_cmd_internal.h"

void console_cmd_register() {
    zod_command_register(COMMAND_GROUP_USER_DEFINED, "log-hook-register",
                         console_cmd_log_hook_register);
    zod_command_register(COMMAND_GROUP_USER_DEFINED, "log-hook-unregister",
                         console_cmd_log_hook_unregister);

    zod_command_register(COMMAND_GROUP_USER_DEFINED, "clear-console",
                         console_cmd_clear_console);
}

#endif
