#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ngine.lib/command.h>
#include "cmd_manager_internal.h"
#include "../../config.h"
#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

// reload-config-file
command_execute_result sys_cmd_priv_reload_config_file(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "reload-config-file: takes no arguments",
        };
    }

    if (!config_priv_reload_from_file(&g_ctx.config)) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "reload-config-file: failed to reload. See log for details",
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

// show-commands
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

#ifndef SYS_CMD_SET_CONFIG_VALUE_LEN
#define SYS_CMD_SET_CONFIG_VALUE_LEN 128
#endif

#ifndef SYS_CMD_CONFIG_BUFFER_LEN
#define SYS_CMD_CONFIG_BUFFER_LEN (CVAR_NAME_MAX + SYS_CMD_SET_CONFIG_VALUE_LEN + 32)
#endif

// set-config
command_execute_result sys_cmd_priv_set_config(int argc, char **argv) {
    static __thread char buf[SYS_CMD_CONFIG_BUFFER_LEN];

    if (argc < 2) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "usage: set-config <cvar_name> <value>",
        };
    }

    const char *name  = argv[0];
    cvar_table *cvars = &g_ctx.config.cvars;
    cvar       *v     = cvar_get(cvars, name);
    if (!v) {
        snprintf(buf, sizeof(buf), "set-config: unknown cvar '%s'", name);
        return (command_execute_result){.type = COMMAND_RESULT_ERROR, .value.str = buf};
    }

    // argv tokens joined with spaces — the shared parser rejects multi-token
    // garbage for int/float/bool on its own (no token fully matches), so no
    // extra "too many args" check is needed here except for CVAR_STRING.
    char   value_buf[SYS_CMD_SET_CONFIG_VALUE_LEN + 2];
    size_t pos = v->type == CVAR_STRING ? 1 : 0;
    for (int i = 1; i < argc; i++) {
        int n = snprintf(value_buf + pos, sizeof(value_buf) - pos, "%s%s",
                         i > 1 ? " " : "", argv[i]);
        if (n > 0) pos += (size_t)n;
        if (pos >= sizeof(value_buf) - 1) {
            pos = sizeof(value_buf) - 2;
            break;
        }
    }
    // Force CVAR_STRING through cvar_parse_and_set_named's quote-stripping path
    // regardless of content — otherwise a numeric-looking value ("123") would
    // get inferred as CVAR_INT and rejected against the string cvar's type.
    if (v->type == CVAR_STRING) {
        value_buf[0]       = '"';
        value_buf[pos]     = '"';
        value_buf[pos + 1] = '\0';
    }

    if (!cvar_parse_and_set_named(name, value_buf, cvars)) {
        snprintf(buf, sizeof(buf), "set-config: failed to set '%s'. See log for details",
                 name);
        return (command_execute_result){.type = COMMAND_RESULT_ERROR, .value.str = buf};
    }

    switch (v->type) {
        case CVAR_INT:
            snprintf(buf, sizeof(buf), "set-config %s=%d", name, v->value.i);
            break;
        case CVAR_FLOAT:
            snprintf(buf, sizeof(buf), "set-config %s=%f", name, v->value.f);
            break;
        case CVAR_BOOL:
            snprintf(buf, sizeof(buf), "set-config %s=%s", name,
                     v->value.b ? "true" : "false");
            break;
        case CVAR_STRING:
            snprintf(buf, sizeof(buf), "set-config %s=\"%s\"", name, v->value.str.data);
            break;
    }

    zngine_apply_config(true);
    return (command_execute_result){.type = COMMAND_RESULT_STRING, .value.str = buf};
}
// get-config
command_execute_result sys_cmd_priv_get_config(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "usage: get-config <cvar_name>",
        };
    }

    static __thread char buf[SYS_CMD_CONFIG_BUFFER_LEN];
    const char          *name  = argv[0];
    cvar_table          *cvars = &g_ctx.config.cvars;
    cvar                *v     = cvar_get(cvars, name);
    if (!v) {
        snprintf(buf, sizeof(buf), "get-config: unknown cvar '%s'", name);
        return (command_execute_result){.type = COMMAND_RESULT_ERROR, .value.str = buf};
    }

    switch (v->type) {
        case CVAR_INT:
            snprintf(buf, sizeof(buf), "get-config: found %s=%d", name, v->value.i);
            break;
        case CVAR_FLOAT:
            snprintf(buf, sizeof(buf), "get-config: found %s=%f", name, v->value.f);
            break;
        case CVAR_BOOL:
            snprintf(buf, sizeof(buf), "get-config: found %s=%s", name,
                     v->value.b ? "true" : "false");
            break;
        case CVAR_STRING:
            snprintf(buf, sizeof(buf), "get-config: found %s=\"%s\"", name,
                     v->value.str.data);
            break;
    }

    return (command_execute_result){.type = COMMAND_RESULT_STRING, .value.str = buf};
}
#ifndef SYS_CMD_LIST_CONFIG_BUFFER_LEN
#define SYS_CMD_LIST_CONFIG_BUFFER_LEN 4096
#endif

// list-config
command_execute_result sys_cmd_priv_list_config(int argc, char **argv) {
    (void)argv;
    if (argc > 0) {
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "list-config: takes no arguments",
        };
    }

    cvar_table *cvars = &g_ctx.config.cvars;
    if (cvars->size == 0) return (command_execute_result){.type = COMMAND_RESULT_VOID};

    static __thread char buf[SYS_CMD_LIST_CONFIG_BUFFER_LEN];
    size_t               pos = 0;

    for (size_t i = 0; i < cvars->size; i++) {
        const cvar *cv = &cvars->data[i];
        sys_cmd_priv_buf_append(buf, sizeof(buf), &pos, "%s %s", cv->name,
                                cvar_type_to_string(cv->type));

        const cvar_constraint *c = cvar_find_constraint(cvars, cv->name);
        bool has_range           = c && (c->range.has_min || c->range.has_max) &&
                                   (cv->type == CVAR_INT || cv->type == CVAR_FLOAT);
        if (has_range) {
            char min_buf[24], max_buf[24];
            if (cv->type == CVAR_INT) {
                snprintf(min_buf, sizeof(min_buf), c->range.has_min ? "%d" : "-inf",
                         c->range.min.i);
                snprintf(max_buf, sizeof(max_buf), c->range.has_max ? "%d" : "inf",
                         c->range.max.i);
            } else {
                snprintf(min_buf, sizeof(min_buf), c->range.has_min ? "%g" : "-inf",
                         (double)c->range.min.f);
                snprintf(max_buf, sizeof(max_buf), c->range.has_max ? "%g" : "inf",
                         (double)c->range.max.f);
            }
            sys_cmd_priv_buf_append(buf, sizeof(buf), &pos, " [%s,%s]", min_buf, max_buf);
        }
        sys_cmd_priv_buf_append(buf, sizeof(buf), &pos, "\n");
    }

    if (pos > 0 && buf[pos - 1] == '\n') buf[pos - 1] = '\0';

    return (command_execute_result){.type = COMMAND_RESULT_STRING, .value.str = buf};
}
#endif
