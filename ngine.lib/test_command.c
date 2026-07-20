#include <stdbool.h>
#include <stddef.h>

#include "../thirdparty/minunit.h"

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
}

int main(void) {
    MU_RUN_SUITE(command_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
