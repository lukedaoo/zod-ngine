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
    mu_check(table.data[0].value.str.cap == 4);
    mu_assert_string_eq("bar", table.data[0].value.str.data);

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
    mu_check(table.data[0].value.str.cap == 4);
    mu_assert_string_eq("bar", table.data[0].value.str.data);

    mu_check(table.data[1].value.i == 12);

    i = 13;
    cvar_set_int(&table, "foo_i", i);

    mu_check(table.size == 2);
    mu_check(table.data[1].value.i == 13);

    cvar_set_string(&table, "foo_s", "a much longer string value");
    mu_check(table.size == 2);
    mu_assert_string_eq("a much longer string value", table.data[0].value.str.data);

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

MU_TEST(test_cvar_typed_get_returns_value) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    cvar_set_int(&table, "an_int", 42);
    cvar_set_float(&table, "a_float", 3.5F);
    cvar_set_bool(&table, "a_bool", true);
    cvar_set_string(&table, "a_str", "hello");

    mu_check(cvar_get_int(&table, "an_int", -1) == 42);
    mu_check(cvar_get_float(&table, "a_float", -1.0F) == 3.5F);
    mu_check(cvar_get_bool(&table, "a_bool", false) == true);
    mu_assert_string_eq("hello", cvar_get_string(&table, "a_str", "FALLBACK"));

    cvar_destroy(&table);
}

MU_TEST(test_cvar_get_missing_returns_fallback) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    mu_check(cvar_get_int(&table, "nope", 99) == 99);
    mu_check(cvar_get_float(&table, "nope", 9.0F) == 9.0F);
    mu_check(cvar_get_bool(&table, "nope", true) == true);

    // string fallback returned verbatim (same pointer)
    const char *fb = "default";
    mu_check(cvar_get_string(&table, "nope", fb) == fb);

    cvar_destroy(&table);
}

MU_TEST(test_cvar_get_type_mismatch_returns_fallback) {
    cvar_table table = {.data = NULL, .size = 0, .capacity = 8};

    cvar_set_int(&table, "an_int", 5);

    // stored is int -> asking any other type returns the fallback
    mu_check(cvar_get_float(&table, "an_int", 7.0F) == 7.0F);
    mu_check(cvar_get_bool(&table, "an_int", true) == true);
    mu_assert_string_eq("fb", cvar_get_string(&table, "an_int", "fb"));

    // asking for the right type still works
    mu_check(cvar_get_int(&table, "an_int", -1) == 5);

    cvar_destroy(&table);
}

MU_TEST(test_cvar_get_null_table_returns_fallback) {
    mu_check(cvar_get(NULL, "x") == NULL);
    mu_check(cvar_get_int(NULL, "x", 7) == 7);
    mu_check(cvar_get_float(NULL, "x", 1.5F) == 1.5F);
    mu_check(cvar_get_bool(NULL, "x", true) == true);
    mu_assert_string_eq("fb", cvar_get_string(NULL, "x", "fb"));
}

MU_TEST(test_cvar_set_grows_capacity) {
    cvar_table table = {0};

    char name[32];
    for (int n = 0; n < 50; n++) {
        snprintf(name, sizeof(name), "var_%d", n);
        mu_check(cvar_set_int(&table, name, n));
    }

    mu_check(table.size == 50);
    mu_check(table.capacity >= 50);

    // every value still retrievable after the reallocs
    for (int n = 0; n < 50; n++) {
        snprintf(name, sizeof(name), "var_%d", n);
        mu_check(cvar_get_int(&table, name, -1) == n);
    }

    cvar_destroy(&table);
}

MU_TEST(test_cvar_set_string_reuses_buffer) {
    cvar_table table = {0};

    // first set allocates a buffer sized to the long string
    cvar_set_string(&table, "k", "a fairly long string value");
    cvar_t *cv   = cvar_get(&table, "k");
    size_t  cap0 = cv->value.str.cap;
    char   *buf0 = cv->value.str.data;

    // shorter value fits -> buffer reused, cap and pointer unchanged
    cvar_set_string(&table, "k", "short");
    mu_check(cv->value.str.cap == cap0);
    mu_check(cv->value.str.data == buf0);
    mu_assert_string_eq("short", cvar_get_string(&table, "k", ""));

    // value longer than cap -> reallocates, cap grows
    cvar_set_string(&table, "k", "an even longer string value than the first one");
    mu_check(cv->value.str.cap > cap0);
    mu_assert_string_eq("an even longer string value than the first one",
                        cvar_get_string(&table, "k", ""));

    cvar_destroy(&table);
}

MU_TEST(test_cvar_set_invalid_args) {
    cvar_table table = {0};

    mu_check(!cvar_set_int(NULL, "x", 1));
    mu_check(!cvar_set_int(&table, NULL, 1));
    mu_check(!cvar_set_string(&table, "x", NULL));

    // name at/over CVAR_NAME_MAX is rejected
    char long_name[CVAR_NAME_MAX + 8];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    mu_check(!cvar_set_int(&table, long_name, 1));

    mu_check(table.size == 0);

    cvar_destroy(&table);
}

MU_TEST(test_cvar_replace_updates_value_via_getter) {
    cvar_table table = {0};

    cvar_set_string(&table, "k", "short");
    cvar_set_string(&table, "k", "a considerably longer value");

    mu_check(table.size == 1);

    mu_assert_string_eq("a considerably longer value", cvar_get_string(&table, "k", ""));

    cvar_destroy(&table);
}

MU_TEST(test_cvar_destroy_null_safe) { cvar_destroy(NULL); }

MU_TEST(test_cvar_override) {
    cvar_table table = {0};

    cvar_set_string(&table, "log.level", "debug");
    mu_check(table.size == 1);
    mu_check(table.data[0].type == CVAR_STRING);

    cvar_set_int(&table, "log.level", 2);
    mu_check(table.size == 1);
    mu_check(table.data[0].type == CVAR_INT);
}

MU_TEST_SUITE(cvar_suite) {
    MU_RUN_TEST(test_cvar_set_insert);
    MU_RUN_TEST(test_cvar_set_insert_with_neg);
    MU_RUN_TEST(test_cvar_set_replace);
    MU_RUN_TEST(test_cvar_get);
    MU_RUN_TEST(test_cvar_typed_get_returns_value);
    MU_RUN_TEST(test_cvar_get_missing_returns_fallback);
    MU_RUN_TEST(test_cvar_get_type_mismatch_returns_fallback);
    MU_RUN_TEST(test_cvar_get_null_table_returns_fallback);
    MU_RUN_TEST(test_cvar_set_grows_capacity);
    MU_RUN_TEST(test_cvar_set_string_reuses_buffer);
    MU_RUN_TEST(test_cvar_set_invalid_args);
    MU_RUN_TEST(test_cvar_replace_updates_value_via_getter);
    MU_RUN_TEST(test_cvar_destroy_null_safe);
    MU_RUN_TEST(test_cvar_override);
}

int main(void) {
    MU_RUN_SUITE(cvar_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
