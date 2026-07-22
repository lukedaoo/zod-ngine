#include "../../thirdparty/minunit.h"

#include <stdbool.h>

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

MU_TEST(test_init_registers_default_system_commands) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    mu_assert_int_eq(2, (int)mgr.table.system_commands.header.size);
    mu_check(command_table_get(&mgr.table, COMMAND_GROUP_SYSTEM, "help") != NULL);
    mu_check(command_table_get(&mgr.table, COMMAND_GROUP_SYSTEM, "reload-config") != NULL);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_destroy_null_safe) { cmd_manager_priv_destroy(NULL); }

MU_TEST_SUITE(cmd_manager_init_suite) {
    MU_RUN_TEST(test_init_registers_default_system_commands);
    MU_RUN_TEST(test_destroy_null_safe);
}

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
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") == true);
    mu_check(command_table_get(&mgr.table, COMMAND_GROUP_USER_DEFINED, "foo") == NULL);
    cmd_manager_priv_destroy(&mgr);
}

MU_TEST(test_unregister_idempotent) {
    cmd_manager mgr = {0};
    cmd_manager_priv_init(&mgr);
    cmd_manager_priv_register(&mgr, COMMAND_GROUP_USER_DEFINED, "foo", mock_handler);
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") == true);
    mu_check(cmd_manager_priv_unregister(&mgr, COMMAND_GROUP_USER_DEFINED, "foo") == false);
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

MU_TEST(test_sys_cmd_help_returns_string) {
    command_execute_result res = sys_cmd_priv_help(0, NULL);
    mu_check(res.type == COMMAND_RESULT_STRING);
}

MU_TEST(test_sys_cmd_reload_config_rejects_args) {
    char                   *argv[] = {"x"};
    command_execute_result res    = sys_cmd_priv_reload_config(1, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
    mu_assert_string_eq("reload-config: takes no arguments", res.value.str);
}

MU_TEST_SUITE(sys_cmd_suite) {
    MU_RUN_TEST(test_sys_cmd_help_returns_string);
    MU_RUN_TEST(test_sys_cmd_reload_config_rejects_args);
}

int main(void) {
    MU_RUN_SUITE(cmd_manager_init_suite);
    MU_RUN_SUITE(cmd_manager_register_suite);
    MU_RUN_SUITE(cmd_manager_execute_suite);
    MU_RUN_SUITE(sys_cmd_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
