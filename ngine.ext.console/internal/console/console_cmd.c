#ifdef ZOD_NGINE_IMPLEMENTATION

#include "console_internal.h"

#if ZOD_CONSOLE_ENABLED

#include <ngine.lib/command.h>
#include <ngine.core/zod_ngine.h>

#include "console_cmd_internal.h"

void console_cmd_priv_register() {
    zngine_command_register(COMMAND_GROUP_USER_DEFINED, "log-hook-register",
                         console_cmd_priv_register_log_hook);
    zngine_command_register(COMMAND_GROUP_USER_DEFINED, "log-hook-unregister",
                         console_cmd_priv_unregister_log_hook);

    zngine_command_register(COMMAND_GROUP_USER_DEFINED, "clear-console",
                         console_cmd_priv_clear_console);
}

#endif  // ZOD_CONSOLE_ENABLED

#endif  // ZOD_NGINE_IMPLEMENTATION
