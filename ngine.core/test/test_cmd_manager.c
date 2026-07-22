#include "../../thirdparty/minunit.h"

#include <stdbool.h>
#include <string.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static command_execute_result mock_handler(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){.type = COMMAND_RESULT_VOID};
}

static command_execute_result mock_handler_alt(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){.type = COMMAND_RESULT_STRING, .value.str = "alt"};
}

MU_TEST(test_destroy_null_safe) { cmd_manager_priv_destroy(NULL); }

MU_TEST_SUITE(cmd_manager_init_suite) { MU_RUN_TEST(test_destroy_null_safe); }

MU_TEST(test_register_user_defined_command) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    bool ok =
         cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler);
    mu_check(ok == true);
    mu_check(command_table_get(&mgr.table, COMMAND_GROUP_USER_DEFINED, "foo") != NULL);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_register_null_mgr_safe) {
    mu_check(cmd_manager_priv_register(NULL, COMMAND_GROUP_SYSTEM, "foo", mock_handler) ==
             false);
}

MU_TEST(test_unregister_removes_command) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler);
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") ==
             true);
    mu_check(command_table_get(&mgr.table, COMMAND_GROUP_USER_DEFINED, "foo") == NULL);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_unregister_idempotent) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler);
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") ==
             true);
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") ==
             false);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_unregister_null_mgr_safe) {
    mu_check(cmd_manager_priv_unregister(NULL, COMMAND_GROUP_SYSTEM, "foo") == false);
}

MU_TEST_SUITE(cmd_manager_register_suite) {
    MU_RUN_TEST(test_register_user_defined_command);
    MU_RUN_TEST(test_register_null_mgr_safe);
    MU_RUN_TEST(test_unregister_removes_command);
    MU_RUN_TEST(test_unregister_idempotent);
    MU_RUN_TEST(test_unregister_null_mgr_safe);
}

MU_TEST(test_execute_routes_to_system_group) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_SYSTEM, "foo", mock_handler);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler_alt);

    command_execute_result res =
         cmd_manager_priv_execute(&mgr, COMMAND_GROUP_SYSTEM, "foo", 0, NULL);
    mu_check(res.type == COMMAND_RESULT_VOID);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_execute_routes_to_user_defined_group) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_SYSTEM, "foo", mock_handler);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler_alt);

    command_execute_result res =
         cmd_manager_priv_execute(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", 0, NULL);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("alt", res.value.str);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_execute_not_found) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    command_execute_result res =
         cmd_manager_priv_execute(&mgr, COMMAND_GROUP_SYSTEM, "missing", 0, NULL);
    mu_check(res.type == COMMAND_RESULT_COMMAND_NOT_FOUND);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_execute_null_mgr_safe) {
    command_execute_result res =
         cmd_manager_priv_execute(NULL, COMMAND_GROUP_SYSTEM, "foo", 0, NULL);
    mu_check(res.type == COMMAND_RESULT_COMMAND_NOT_FOUND);
}

MU_TEST_SUITE(cmd_manager_execute_suite) {
    MU_RUN_TEST(test_execute_routes_to_system_group);
    MU_RUN_TEST(test_execute_routes_to_user_defined_group);
    MU_RUN_TEST(test_execute_not_found);
    MU_RUN_TEST(test_execute_null_mgr_safe);
}

MU_TEST(test_sys_cmd_reload_config_file_rejects_args) {
    char                  *argv[] = {"x"};
    command_execute_result res    = sys_cmd_priv_reload_config_file(1, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
    mu_assert_string_eq("reload-config-file: takes no arguments", res.value.str);
}

MU_TEST(test_sys_cmd_show_commands_lists_both_groups) {
    g_ctx = (engine_context){0};
    cmd_manager_priv_init(&g_ctx.cmd_manager);
    cmd_manager_priv_register(&g_ctx.cmd_manager, COMMAND_GROUP_USER_DEFINED, "foo",
                              mock_handler);

    command_execute_result res = sys_cmd_priv_show_commands(0, NULL);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_check(strstr(res.value.str,
                    "system [reload-config-file, show-commands, set-config, get-config, "
                    "list-config]") != NULL);
    mu_check(strstr(res.value.str, "user [foo]") != NULL);
    mu_check(res.value.str[strlen(res.value.str) - 1] != '\n');

    cmd_manager_priv_destroy(&g_ctx.cmd_manager);
}

MU_TEST(test_sys_cmd_show_commands_filters_by_group) {
    g_ctx = (engine_context){0};
    cmd_manager_priv_init(&g_ctx.cmd_manager);
    cmd_manager_priv_register(&g_ctx.cmd_manager, COMMAND_GROUP_USER_DEFINED, "foo",
                              mock_handler);

    char                  *argv[] = {"user"};
    command_execute_result res    = sys_cmd_priv_show_commands(1, argv);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("user [foo]", res.value.str);

    cmd_manager_priv_destroy(&g_ctx.cmd_manager);
}

MU_TEST(test_sys_cmd_show_commands_rejects_unknown_group) {
    char                  *argv[] = {"bogus"};
    command_execute_result res    = sys_cmd_priv_show_commands(1, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
}

MU_TEST(test_sys_cmd_show_commands_rejects_extra_args) {
    char                  *argv[] = {"system", "extra"};
    command_execute_result res    = sys_cmd_priv_show_commands(2, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
}

MU_TEST(test_sys_cmd_set_config_rejects_missing_args) {
    char                  *argv[] = {"window.width"};
    command_execute_result res    = sys_cmd_priv_set_config(1, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
}

MU_TEST(test_sys_cmd_set_config_rejects_unknown_cvar) {
    g_ctx                         = (engine_context){0};
    char                  *argv[] = {"does.not.exist", "1"};
    command_execute_result res    = sys_cmd_priv_set_config(2, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
}

MU_TEST(test_sys_cmd_set_config_sets_int) {
    g_ctx = (engine_context){0};
    cvar_set_int(&g_ctx.config.cvars, "window.width", 800);

    char                  *argv[] = {"window.width", "1280"};
    command_execute_result res    = sys_cmd_priv_set_config(2, argv);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("set-config window.width=1280", res.value.str);
    mu_assert_int_eq(1280, cvar_get_int(&g_ctx.config.cvars, "window.width", -1));

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_set_config_coerces_bare_int_for_float) {
    g_ctx = (engine_context){0};
    cvar_set_float(&g_ctx.config.cvars, "console.input_gap", 0.0f);
    static const cvar_constraint c = {.name     = "console.input_gap",
                                      .expected = CVAR_FLOAT};
    cvar_add_schema(&g_ctx.config.cvars, &c, 1);

    char                  *argv[] = {"console.input_gap", "8"};
    command_execute_result res    = sys_cmd_priv_set_config(2, argv);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_check(cvar_get_float(&g_ctx.config.cvars, "console.input_gap", -1.0f) == 8.0f);

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_set_config_rejects_bad_int) {
    g_ctx = (engine_context){0};
    cvar_set_int(&g_ctx.config.cvars, "window.width", 800);
    static const cvar_constraint c = {.name = "window.width", .expected = CVAR_INT};
    cvar_add_schema(&g_ctx.config.cvars, &c, 1);

    char                  *argv[] = {"window.width", "not-a-number"};
    command_execute_result res    = sys_cmd_priv_set_config(2, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
    mu_assert_int_eq(800, cvar_get_int(&g_ctx.config.cvars, "window.width", -1));

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_set_config_sets_bool) {
    g_ctx = (engine_context){0};
    cvar_set_bool(&g_ctx.config.cvars, "window.vsync", false);

    char                  *argv[] = {"window.vsync", "true"};
    command_execute_result res    = sys_cmd_priv_set_config(2, argv);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_check(cvar_get_bool(&g_ctx.config.cvars, "window.vsync", false) == true);

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_set_config_sets_string_with_spaces) {
    g_ctx = (engine_context){0};
    cvar_set_string(&g_ctx.config.cvars, "window.title", "old");

    char                  *argv[] = {"window.title", "Hello", "World"};
    command_execute_result res    = sys_cmd_priv_set_config(3, argv);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("set-config window.title=\"Hello World\"", res.value.str);
    mu_assert_string_eq("Hello World",
                        cvar_get_string(&g_ctx.config.cvars, "window.title", ""));

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_set_config_string_accepts_numeric_literal) {
    g_ctx = (engine_context){0};
    cvar_set_string(&g_ctx.config.cvars, "window.title", "old");

    char                  *argv[] = {"window.title", "123"};
    command_execute_result res    = sys_cmd_priv_set_config(2, argv);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("123", cvar_get_string(&g_ctx.config.cvars, "window.title", ""));

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_list_config_rejects_args) {
    char                  *argv[] = {"extra"};
    command_execute_result res    = sys_cmd_priv_list_config(1, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
}

MU_TEST(test_sys_cmd_list_config_empty_table_is_void) {
    g_ctx                      = (engine_context){0};
    command_execute_result res = sys_cmd_priv_list_config(0, NULL);
    mu_check(res.type == COMMAND_RESULT_VOID);
}

MU_TEST(test_sys_cmd_list_config_shows_range_when_constrained) {
    g_ctx = (engine_context){0};
    cvar_set_int(&g_ctx.config.cvars, "engine.target_fps", 60);
    static const cvar_constraint c = {.name     = "engine.target_fps",
                                      .expected = CVAR_INT,
                                      .range    = {.has_min = true, .min.i = 1}};
    cvar_add_schema(&g_ctx.config.cvars, &c, 1);

    command_execute_result res = sys_cmd_priv_list_config(0, NULL);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("engine.target_fps int [1,inf]", res.value.str);

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_sys_cmd_list_config_omits_brackets_when_unconstrained) {
    g_ctx = (engine_context){0};
    cvar_set_bool(&g_ctx.config.cvars, "debug.enabled", true);

    command_execute_result res = sys_cmd_priv_list_config(0, NULL);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("debug.enabled bool", res.value.str);

    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST_SUITE(sys_cmd_suite) {
    MU_RUN_TEST(test_sys_cmd_reload_config_file_rejects_args);
    MU_RUN_TEST(test_sys_cmd_show_commands_lists_both_groups);
    MU_RUN_TEST(test_sys_cmd_show_commands_filters_by_group);
    MU_RUN_TEST(test_sys_cmd_show_commands_rejects_unknown_group);
    MU_RUN_TEST(test_sys_cmd_show_commands_rejects_extra_args);
    MU_RUN_TEST(test_sys_cmd_set_config_rejects_missing_args);
    MU_RUN_TEST(test_sys_cmd_set_config_rejects_unknown_cvar);
    MU_RUN_TEST(test_sys_cmd_set_config_sets_int);
    MU_RUN_TEST(test_sys_cmd_set_config_coerces_bare_int_for_float);
    MU_RUN_TEST(test_sys_cmd_set_config_rejects_bad_int);
    MU_RUN_TEST(test_sys_cmd_set_config_sets_bool);
    MU_RUN_TEST(test_sys_cmd_set_config_sets_string_with_spaces);
    MU_RUN_TEST(test_sys_cmd_set_config_string_accepts_numeric_literal);
    MU_RUN_TEST(test_sys_cmd_list_config_rejects_args);
    MU_RUN_TEST(test_sys_cmd_list_config_empty_table_is_void);
    MU_RUN_TEST(test_sys_cmd_list_config_shows_range_when_constrained);
    MU_RUN_TEST(test_sys_cmd_list_config_omits_brackets_when_unconstrained);
}

int main(void) {
    MU_RUN_SUITE(cmd_manager_init_suite);
    MU_RUN_SUITE(cmd_manager_register_suite);
    MU_RUN_SUITE(cmd_manager_execute_suite);
    MU_RUN_SUITE(sys_cmd_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
