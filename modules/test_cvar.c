#include "../lib/minunit.h"

#define CVAR_IMPLEMENTATION
#include "cvar.h"

MU_TEST(test_cvar_set_insert) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    cvar_set_string(&table, "foo_s", "bar");
    int i = 12;
    cvar_set_int(&table, "foo_i", i);
    float f = 1.1F;
    cvar_set_float(&table, "foo_f", f);
    bool b = true;
    cvar_set_bool(&table, "foo_b_t1", b);
    bool b2 = 1;
    cvar_set_bool(&table, "foo_b_t2", b2);
    bool b3 = false;
    cvar_set_bool(&table, "foo_b_f1", b3);
    bool b4 = 0;
    cvar_set_bool(&table, "foo_b_f2", b4);

    mu_check(table.size == 7);

    mu_assert_string_eq("foo_s", table.data[0].name);
    mu_check(table.data[0].type == CVAR_STRING);
    mu_check(table.data[0].capacity == 4);
    mu_assert_string_eq("bar", table.data[0].value.s);

    mu_check(table.data[1].value.i == 12);

    mu_check(table.data[2].value.f == 1.1F);

    mu_check(table.data[3].value.b == true);
    mu_check(table.data[4].value.b == true);
    mu_check(table.data[5].value.b == false);
    mu_check(table.data[6].value.b == false);

    cvar_destroy(&table);
}

MU_TEST(test_cvar_set_insert_with_neg) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    int i = -12;
    cvar_set_int(&table, "foo_i", i);
    float f = -1.1F;
    cvar_set_float(&table, "foo_f", f);

    mu_check(table.size == 2);

    mu_check(table.data[0].value.i == -12);

    mu_check(table.data[1].value.f == -1.1F);

    cvar_destroy(&table);
}
MU_TEST(test_cvar_set_replace) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    cvar_set_string(&table, "foo_s", "bar");
    int i = 12;
    cvar_set_int(&table, "foo_i", i);

    mu_check(table.size == 2);

    mu_assert_string_eq("foo_s", table.data[0].name);
    mu_check(table.data[0].type == CVAR_STRING);
    mu_check(table.data[0].capacity == 4);
    mu_assert_string_eq("bar", table.data[0].value.s);

    mu_check(table.data[1].value.i == 12);

    i = 13;
    cvar_set_int(&table, "foo_i", i);

    mu_check(table.size == 2);
    mu_check(table.data[1].value.i == 13);

    cvar_set_string(&table, "foo_s", "a much longer string value");
    mu_check(table.size == 2);
    mu_assert_string_eq("a much longer string value", table.data[0].value.s);

    cvar_destroy(&table);
}

MU_TEST(test_cvar_get) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    int i = 12;
    cvar_set_int(&table, "foo_i", i);
    cvar_set_string(&table, "foo_s", "bar");

    cvar_t *cv = cvar_get(&table, "foo_i");
    mu_check(cv != NULL);
    mu_check(cv->value.i == 12);

    mu_check(cvar_get(&table, "missing") == NULL);

    cvar_destroy(&table);
}

MU_TEST_SUITE(cvar_suite) {
    MU_RUN_TEST(test_cvar_set_insert);
    MU_RUN_TEST(test_cvar_set_insert_with_neg);
    MU_RUN_TEST(test_cvar_set_replace);
    MU_RUN_TEST(test_cvar_get);
}

int main(void) {
    MU_RUN_SUITE(cvar_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
