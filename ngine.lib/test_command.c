#include <stdbool.h>
#include <stddef.h>

#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#define ARRAY_LIST_IMPLEMENTATION
#define COMMAND_IMPLEMENTATION
#include "command.h"

MU_TEST(test_init_null_safe) { command_table_init(NULL); }

MU_TEST(test_init_default_capacity) {
    command_table table = {0};
    command_table_init(&table);
    mu_assert_int_eq(0, (int)table.system_commands.header.size);
    mu_assert_int_eq(0, (int)table.user_defined_commands.header.size);
    mu_assert_int_eq(COMMAND_TABLE_SYSTEM_COMMAND_INIT_SIZE,
                     (int)table.system_commands.header.capacity);
    mu_assert_int_eq(COMMAND_TABLE_USER_COMMAND_INITIAL_CAPACITY,
                     (int)table.user_defined_commands.header.capacity);
    mu_assert_int_eq((int)sizeof(command),
                     (int)table.system_commands.header.element_size);
    mu_check(table.system_commands.data != NULL);
    mu_check(table.user_defined_commands.data != NULL);
    command_table_destroy(&table);
}

MU_TEST(test_init_with_capacity) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 8);
    mu_assert_int_eq(4, (int)table.system_commands.header.capacity);
    mu_assert_int_eq(8, (int)table.user_defined_commands.header.capacity);
    mu_assert_int_eq(0, (int)table.system_commands.header.size);
    mu_assert_int_eq(0, (int)table.user_defined_commands.header.size);
    command_table_destroy(&table);
}

MU_TEST(test_init_with_capacity_null_safe) {
    command_table_init_with_capacity(NULL, 4, 8);
}

MU_TEST(test_destroy_null_safe) { command_table_destroy(NULL); }

MU_TEST(test_destroy_frees_buffers) {
    command_table table = {0};
    command_table_init(&table);
    command_table_destroy(&table);
    mu_check(table.system_commands.data == NULL);
    mu_check(table.user_defined_commands.data == NULL);
    mu_assert_int_eq(0, (int)table.system_commands.header.capacity);
    command_table_destroy(&table);
}

MU_TEST(test_reserve_system_null_safe) {
    command_table_reserve_system_commands(NULL, 10);
}

MU_TEST(test_reserve_system_zero_noop) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_reserve_system_commands(&table, 0);
    mu_assert_int_eq(4, (int)table.system_commands.header.capacity);
    command_table_destroy(&table);
}

MU_TEST(test_reserve_system_grows) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_reserve_system_commands(&table, 100);
    mu_check(table.system_commands.header.capacity >= 100);
    mu_assert_int_eq(4, (int)table.user_defined_commands.header.capacity);
    command_table_destroy(&table);
}

MU_TEST(test_reserve_system_within_capacity_noop) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 16, 4);
    command_table_reserve_system_commands(&table, 8);  // 0 + 8 <= 16
    mu_assert_int_eq(16, (int)table.system_commands.header.capacity);
    command_table_destroy(&table);
}

MU_TEST(test_reserve_user_null_safe) {
    command_table_reserve_user_defined_commands(NULL, 10);
}

MU_TEST(test_reserve_user_zero_noop) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_reserve_user_defined_commands(&table, 0);
    mu_assert_int_eq(4, (int)table.user_defined_commands.header.capacity);
    command_table_destroy(&table);
}

MU_TEST(test_reserve_user_grows) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_reserve_user_defined_commands(&table, 100);
    mu_check(table.user_defined_commands.header.capacity >= 100);
    mu_assert_int_eq(4, (int)table.system_commands.header.capacity);
    command_table_destroy(&table);
}

command_execute_result handler(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){.type = COMMAND_RESULT_VOID};
}

command_execute_result handler_sum(int argc, char **argv) {
    if (argc != 2)
        return (command_execute_result){
             .type      = COMMAND_RESULT_ERROR,
             .value.str = "sum: requires exactly 2 arguments"  //
        };
    return (command_execute_result){
         .type    = COMMAND_RESULT_INT,
         .value.i = atoi(argv[0]) + atoi(argv[1])  //
    };
}

command_execute_result handler_float(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){
         .type    = COMMAND_RESULT_FLOAT,
         .value.f = 3.14F  //
    };
}

command_execute_result handler_string(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){
         .type      = COMMAND_RESULT_STRING,
         .value.str = "hello"  //
    };
}

static int ptr_target = 42;

command_execute_result handler_ptr(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return (command_execute_result){
         .type      = COMMAND_RESULT_PTR,
         .value.ptr = &ptr_target  //
    };
}

MU_TEST(test_register_null_safe) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    bool res = command_table_register(&table, COMMAND_GROUP_USER_DEFINED, NULL, handler);
    mu_check(res == false);
    mu_assert_int_eq(0, (int)table.user_defined_commands.header.size);
    command_table_destroy(&table);
}

MU_TEST(test_register_user_defined) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_USER_DEFINED, "foo", handler);
    mu_assert_int_eq(1, (int)table.user_defined_commands.header.size);
    command_table_destroy(&table);
}

MU_TEST(test_register_system_command) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    mu_assert_int_eq(1, (int)table.system_commands.header.size);
    command_table_destroy(&table);
}

MU_TEST(test_register_retrievable) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    command *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "foo");
    mu_check(cmd != NULL);
    mu_check(cmd->handler == handler);
    mu_assert_string_eq("foo", cmd->name);
    command_table_destroy(&table);
}

MU_TEST(test_register_duplicate_rejected) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    bool res1 = command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    bool res2 = command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    mu_check(res1 == true);
    mu_check(res2 == false);
    mu_assert_int_eq(1, (int)table.system_commands.header.size);
    command_table_destroy(&table);
}

MU_TEST(test_register_null_table) {
    bool res = command_table_register(NULL, COMMAND_GROUP_SYSTEM, "foo", handler);
    mu_check(res == false);
}

MU_TEST(test_register_null_handler) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    bool res = command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", NULL);
    mu_check(res == false);
    mu_assert_int_eq(0, (int)table.system_commands.header.size);
    command_table_destroy(&table);
}

MU_TEST(test_register_empty_name) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    bool res = command_table_register(&table, COMMAND_GROUP_SYSTEM, "", handler);
    mu_check(res == true);
    mu_check(command_table_get(&table, COMMAND_GROUP_SYSTEM, "") != NULL);
    command_table_destroy(&table);
}

MU_TEST(test_get_found_system) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    mu_check(command_table_get(&table, COMMAND_GROUP_SYSTEM, "foo") != NULL);
    command_table_destroy(&table);
}

MU_TEST(test_get_found_user_defined) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_USER_DEFINED, "foo", handler);
    mu_check(command_table_get(&table, COMMAND_GROUP_USER_DEFINED, "foo") != NULL);
    command_table_destroy(&table);
}

MU_TEST(test_get_not_found) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    mu_check(command_table_get(&table, COMMAND_GROUP_SYSTEM, "missing") == NULL);
    command_table_destroy(&table);
}

MU_TEST(test_get_wrong_group) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    mu_check(command_table_get(&table, COMMAND_GROUP_USER_DEFINED, "foo") == NULL);
    command_table_destroy(&table);
}

MU_TEST(test_get_null_table) {
    mu_check(command_table_get(NULL, COMMAND_GROUP_SYSTEM, "foo") == NULL);
}

MU_TEST(test_get_null_name) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    mu_check(command_table_get(&table, COMMAND_GROUP_SYSTEM, NULL) == NULL);
    command_table_destroy(&table);
}

MU_TEST(test_unregister_system_removes_command) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    bool res = command_table_unregister(&table, COMMAND_GROUP_SYSTEM, "foo");
    mu_check(res == true);
    mu_assert_int_eq(0, (int)table.system_commands.header.size);
    mu_check(command_table_get(&table, COMMAND_GROUP_SYSTEM, "foo") == NULL);
    command_table_destroy(&table);
}

MU_TEST(test_unregister_user_defined_removes_command) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_USER_DEFINED, "foo", handler);
    bool res = command_table_unregister(&table, COMMAND_GROUP_USER_DEFINED, "foo");
    mu_check(res == true);
    mu_assert_int_eq(0, (int)table.user_defined_commands.header.size);
    mu_check(command_table_get(&table, COMMAND_GROUP_USER_DEFINED, "foo") == NULL);
    command_table_destroy(&table);
}

MU_TEST(test_unregister_not_found) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    bool res = command_table_unregister(&table, COMMAND_GROUP_SYSTEM, "missing");
    mu_check(res == false);
    command_table_destroy(&table);
}

MU_TEST(test_unregister_double_idempotent) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "foo", handler);
    bool res1 = command_table_unregister(&table, COMMAND_GROUP_SYSTEM, "foo");
    bool res2 = command_table_unregister(&table, COMMAND_GROUP_SYSTEM, "foo");
    mu_check(res1 == true);
    mu_check(res2 == false);
    command_table_destroy(&table);
}

MU_TEST(test_unregister_null_table) {
    mu_check(command_table_unregister(NULL, COMMAND_GROUP_SYSTEM, "foo") == false);
}

MU_TEST(test_unregister_null_name) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    mu_check(command_table_unregister(&table, COMMAND_GROUP_SYSTEM, NULL) == false);
    command_table_destroy(&table);
}

MU_TEST(test_execute_null_command) {
    command_execute_result res = command_execute(NULL, 0, NULL);
    mu_check(res.type == COMMAND_RESULT_COMMAND_NOT_FOUND);
    mu_check(res.value.str != NULL);
}

MU_TEST(test_execute_void) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "noop", handler);
    command               *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "noop");
    command_execute_result res = command_execute(cmd, 0, NULL);
    mu_check(res.type == COMMAND_RESULT_VOID);
    command_table_destroy(&table);
}

MU_TEST(test_execute_float) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "pi", handler_float);
    command               *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "pi");
    command_execute_result res = command_execute(cmd, 0, NULL);
    mu_check(res.type == COMMAND_RESULT_FLOAT);
    mu_check(res.value.f == 3.14F);
    command_table_destroy(&table);
}

MU_TEST(test_execute_string) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "greet", handler_string);
    command               *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "greet");
    command_execute_result res = command_execute(cmd, 0, NULL);
    mu_check(res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("hello", res.value.str);
    command_table_destroy(&table);
}

MU_TEST(test_execute_ptr) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "addr", handler_ptr);
    command               *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "addr");
    command_execute_result res = command_execute(cmd, 0, NULL);
    mu_check(res.type == COMMAND_RESULT_PTR);
    mu_check(res.value.ptr == &ptr_target);
    command_table_destroy(&table);
}

MU_TEST(test_execute_sum_success) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "sum", handler_sum);
    command               *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "sum");
    char                  *argv[] = {"3", "4"};
    command_execute_result res    = command_execute(cmd, 2, argv);
    mu_check(res.type == COMMAND_RESULT_INT);
    mu_assert_int_eq(7, res.value.i);
    command_table_destroy(&table);
}

MU_TEST(test_execute_sum_wrong_argc) {
    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);
    command_table_register(&table, COMMAND_GROUP_SYSTEM, "sum", handler_sum);
    command               *cmd = command_table_get(&table, COMMAND_GROUP_SYSTEM, "sum");
    char                  *argv[] = {"3"};
    command_execute_result res    = command_execute(cmd, 1, argv);
    mu_check(res.type == COMMAND_RESULT_ERROR);
    mu_assert_string_eq("sum: requires exactly 2 arguments", res.value.str);
    command_table_destroy(&table);
}

MU_TEST_SUITE(command_suite) {
    MU_RUN_TEST(test_init_null_safe);
    MU_RUN_TEST(test_init_default_capacity);
    MU_RUN_TEST(test_init_with_capacity);
    MU_RUN_TEST(test_init_with_capacity_null_safe);
    MU_RUN_TEST(test_destroy_null_safe);
    MU_RUN_TEST(test_destroy_frees_buffers);
    MU_RUN_TEST(test_reserve_system_null_safe);
    MU_RUN_TEST(test_reserve_system_zero_noop);
    MU_RUN_TEST(test_reserve_system_grows);
    MU_RUN_TEST(test_reserve_system_within_capacity_noop);
    MU_RUN_TEST(test_reserve_user_null_safe);
    MU_RUN_TEST(test_reserve_user_zero_noop);
    MU_RUN_TEST(test_reserve_user_grows);
    MU_RUN_TEST(test_register_null_safe);
    MU_RUN_TEST(test_register_user_defined);
    MU_RUN_TEST(test_register_system_command);
}

MU_TEST_SUITE(command_register_suite) {
    MU_RUN_TEST(test_register_retrievable);
    MU_RUN_TEST(test_register_duplicate_rejected);
    MU_RUN_TEST(test_register_null_table);
    MU_RUN_TEST(test_register_null_handler);
    MU_RUN_TEST(test_register_empty_name);
}

MU_TEST_SUITE(command_get_suite) {
    MU_RUN_TEST(test_get_found_system);
    MU_RUN_TEST(test_get_found_user_defined);
    MU_RUN_TEST(test_get_not_found);
    MU_RUN_TEST(test_get_wrong_group);
    MU_RUN_TEST(test_get_null_table);
    MU_RUN_TEST(test_get_null_name);
}

MU_TEST_SUITE(command_unregister_suite) {
    MU_RUN_TEST(test_unregister_system_removes_command);
    MU_RUN_TEST(test_unregister_user_defined_removes_command);
    MU_RUN_TEST(test_unregister_not_found);
    MU_RUN_TEST(test_unregister_double_idempotent);
    MU_RUN_TEST(test_unregister_null_table);
    MU_RUN_TEST(test_unregister_null_name);
}

MU_TEST_SUITE(command_execute_suite) {
    MU_RUN_TEST(test_execute_null_command);
    MU_RUN_TEST(test_execute_void);
    MU_RUN_TEST(test_execute_sum_success);
    MU_RUN_TEST(test_execute_sum_wrong_argc);
    MU_RUN_TEST(test_execute_float);
    MU_RUN_TEST(test_execute_string);
    MU_RUN_TEST(test_execute_ptr);
}

MU_TEST(test_stress_game_session) {
    struct rusage ru_before;
    getrusage(RUSAGE_SELF, &ru_before);

    command_table table = {0};
    command_table_init_with_capacity(&table, 4, 4);  // force growth under load

    const int total = 200;
    char      argv1[2][16], argv2[2][16];

    for (int i = 0; i < total; i++) {
        char name[COMMAND_MAX_NAME_LEN];
        snprintf(name, sizeof(name), "game_cmd_%d", i);
        command_group group =
             (i % 2 == 0) ? COMMAND_GROUP_SYSTEM : COMMAND_GROUP_USER_DEFINED;

        command_execute_result (*chosen_handler)(int, char **);
        switch (i % 5) {
            case 0:
                chosen_handler = handler;
                break;
            case 1:
                chosen_handler = handler_sum;
                break;
            case 2:
                chosen_handler = handler_float;
                break;
            case 3:
                chosen_handler = handler_string;
                break;
            default:
                chosen_handler = handler_ptr;
                break;
        }
        mu_check(command_table_register(&table, group, name, chosen_handler) == true);
    }
    mu_assert_int_eq(100, (int)table.system_commands.header.size);
    mu_assert_int_eq(100, (int)table.user_defined_commands.header.size);

    for (int i = 0; i < total; i++) {
        char name[COMMAND_MAX_NAME_LEN];
        snprintf(name, sizeof(name), "game_cmd_%d", i);
        command_group group =
             (i % 2 == 0) ? COMMAND_GROUP_SYSTEM : COMMAND_GROUP_USER_DEFINED;
        command *cmd = command_table_get(&table, group, name);
        mu_check(cmd != NULL);

        command_execute_result res;
        switch (i % 5) {
            case 0:  // VOID
                res = command_execute(cmd, 0, NULL);
                mu_check(res.type == COMMAND_RESULT_VOID);
                break;
            case 1:  // sum: argc-sensitive — mix real success and missing-arg error
                if (i % 3 == 0) {
                    char *argv[] = {argv1[0]};
                    snprintf(argv1[0], sizeof(argv1[0]), "%d", i);
                    res = command_execute(cmd, 1, argv);
                    mu_check(res.type == COMMAND_RESULT_ERROR);
                    mu_check(res.value.str != NULL);
                } else {
                    char *argv[] = {argv2[0], argv2[1]};
                    snprintf(argv2[0], sizeof(argv2[0]), "%d", i);
                    snprintf(argv2[1], sizeof(argv2[1]), "1");
                    res = command_execute(cmd, 2, argv);
                    mu_check(res.type == COMMAND_RESULT_INT);
                    mu_assert_int_eq(i + 1, res.value.i);
                }
                break;
            case 2:  // FLOAT
                res = command_execute(cmd, 0, NULL);
                mu_check(res.type == COMMAND_RESULT_FLOAT);
                mu_check(res.value.f == 3.14F);
                break;
            case 3:  // STRING
                res = command_execute(cmd, 0, NULL);
                mu_check(res.type == COMMAND_RESULT_STRING);
                mu_assert_string_eq("hello", res.value.str);
                break;
            default:  // PTR
                res = command_execute(cmd, 0, NULL);
                mu_check(res.type == COMMAND_RESULT_PTR);
                break;
        }
    }

    // unknown command — never registered, no crash, clean error
    command *missing = command_table_get(&table, COMMAND_GROUP_SYSTEM, "does_not_exist");
    mu_check(missing == NULL);
    command_execute_result unknown_res = command_execute(missing, 0, NULL);
    mu_check(unknown_res.type == COMMAND_RESULT_COMMAND_NOT_FOUND);

    // duplicate registration mid-load-simulation — rejected, table size untouched
    mu_check(command_table_register(&table, COMMAND_GROUP_SYSTEM, "game_cmd_0",
                                    handler) == false);
    mu_assert_int_eq(100, (int)table.system_commands.header.size);
    mu_assert_int_eq(100, (int)table.user_defined_commands.header.size);

    // state not corrupted — re-fetch/re-execute an already-used command, still
    // deterministic
    command *recheck =
         command_table_get(&table, COMMAND_GROUP_USER_DEFINED, "game_cmd_3");
    mu_check(recheck != NULL);
    command_execute_result recheck_res = command_execute(recheck, 0, NULL);
    mu_check(recheck_res.type == COMMAND_RESULT_STRING);
    mu_assert_string_eq("hello", recheck_res.value.str);

    size_t system_bytes = table.system_commands.header.capacity *
                          table.system_commands.header.element_size;
    size_t user_bytes = table.user_defined_commands.header.capacity *
                        table.user_defined_commands.header.element_size;

    struct rusage ru_after;
    getrusage(RUSAGE_SELF, &ru_after);
    long rss_delta_kb = ru_after.ru_maxrss - ru_before.ru_maxrss;

    printf("[mem] test_stress_game_session:\n");
    printf("  system_commands:       %zu bytes (cap=%zu elems)\n", system_bytes,
          table.system_commands.header.capacity);
    printf("  user_defined_commands: %zu bytes (cap=%zu elems)\n", user_bytes,
          table.user_defined_commands.header.capacity);
    printf("  total array_list heap: %zu bytes\n", system_bytes + user_bytes);
    printf("  peak RSS delta:        %ld KB\n", rss_delta_kb);

    command_table_destroy(&table);
}

MU_TEST_SUITE(command_stress_suite) { MU_RUN_TEST(test_stress_game_session); }

int main(void) {
    MU_RUN_SUITE(command_suite);
    MU_RUN_SUITE(command_register_suite);
    MU_RUN_SUITE(command_get_suite);
    MU_RUN_SUITE(command_unregister_suite);
    MU_RUN_SUITE(command_execute_suite);
    MU_RUN_SUITE(command_stress_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
