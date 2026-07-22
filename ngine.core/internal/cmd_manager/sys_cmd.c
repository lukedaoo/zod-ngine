#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ngine.lib/command.h>
#include "cmd_manager_internal.h"
#include "../../config.h"
#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

command_execute_result sys_cmd_priv_reload_config(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "reload-config: takes no arguments",
        };
    }

    if (!config_priv_reload_from_file(&g_ctx.config)) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "reload-config: failed to reload. See log for details",
        };
    }

    zngine_apply_config(true);
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "config reloaded",
    };
}

#ifndef SYS_CMD_SHOW_COMMANDS_BUFFER_LEN
#define SYS_CMD_SHOW_COMMANDS_BUFFER_LEN 1024
#endif

static void sys_cmd_priv_buf_append(char *buf, size_t len, size_t *pos, const char *fmt,
                                    ...) {
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buf + *pos, len - *pos, fmt, args);
    va_end(args);
    *pos += (size_t)(written > 0 ? written : 0);
    if (*pos >= len) *pos = len - 1;  // snprintf return can exceed remaining space
}

static void sys_cmd_priv_append_group(char *buf, size_t len, size_t *pos,
                                      const char *label, const array_list *list) {
    sys_cmd_priv_buf_append(buf, len, pos, "%s [", label);
    for (size_t i = 0; i < list->header.size; i++) {
        const command *cmd = (const command *)array_list_get(list, i);
        sys_cmd_priv_buf_append(buf, len, pos, "%s%s", i == 0 ? "" : ", ", cmd->name);
    }
    sys_cmd_priv_buf_append(buf, len, pos, "]\n");
}

command_execute_result sys_cmd_priv_show_commands(int argc, char **argv) {
    if (argc > 1) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "show-commands: takes at most one argument",
        };
    }

    bool show_system = argc == 0 || strcmp(argv[0], "system") == 0;
    bool show_user   = argc == 0 || strcmp(argv[0], "user") == 0;
    if (!show_system && !show_user) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "show-commands: expected 'system' or 'user'",
        };
    }

    static __thread char buf[SYS_CMD_SHOW_COMMANDS_BUFFER_LEN];
    size_t               pos = 0;
    if (show_system)
        sys_cmd_priv_append_group(buf, sizeof(buf), &pos, "system",
                                  &g_ctx.cmd_manager.table.system_commands);
    if (show_user)
        sys_cmd_priv_append_group(buf, sizeof(buf), &pos, "user",
                                  &g_ctx.cmd_manager.table.user_defined_commands);

    if (pos > 0 && buf[pos - 1] == '\n') buf[pos - 1] = '\0';

    return (command_execute_result){.type = COMMAND_RESULT_STRING, .value.str = buf};
}

#endif
