#include "../lib/minunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CVAR_IMPLEMENTATION
#include "cvar.h"

#define INI_IMPLEMENTATION
#include "ini.h"

#define SCF_IMPLEMENTATION
#include "scf.h"

#define CVAR_LOAD_IMPLEMENTATION
#include "cvar_load.h"

#ifdef _WIN32
#define TEST_INI "tmp/cvar_load_test.ini"
#else
#define TEST_INI "/tmp/cvar_load_test.ini"
#endif

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    fputs(content, f);
    fclose(f);
}

static bool test_ini_handler(const char *section, const char *key, const char *value,
                             void *user) {
    cvar_table *cvars = user;

#define MATCH(s, k) (strcmp(section, s) == 0 && strcmp(key, k) == 0)
    if (MATCH("test", "value")) {
        int v = atoi(value);
        cvar_set_int(cvars, "test.value", v);
    } else if (MATCH("test", "name")) {
        cvar_set_string(cvars, "test.name", value);
    } else {
        return false;
    }
    return true;
#undef MATCH
}

MU_TEST(test_cvar_force_reload_ini_success) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(ini_parse(TEST_INI, test_ini_handler, &table));

    write_file(TEST_INI, "[test]\nvalue=2\nname=bob\n");
    mu_check(cvar_load_ini(&table, TEST_INI, test_ini_handler, true));

    cvar_t *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 2);

    cvar_t *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("bob", n->value.str.data);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST(test_cvar_reload_ini_success) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(ini_parse(TEST_INI, test_ini_handler, &table));

    write_file(TEST_INI, "[test]\nvalue=2\nname=bob\n");
    mu_check(cvar_load_ini(&table, TEST_INI, test_ini_handler, false));

    cvar_t *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 2);

    cvar_t *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("bob", n->value.str.data);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST(test_cvar_reload_ini_failure_keeps_old) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(ini_parse(TEST_INI, test_ini_handler, &table));

    write_file(TEST_INI, "[test]\nvalue=99\nunknown_key=oops\n");
    mu_check(!cvar_load_ini(&table, TEST_INI, test_ini_handler, true));

    cvar_t *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 1);

    cvar_t *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("alice", n->value.str.data);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST_SUITE(cvar_reload_suite) {
    MU_RUN_TEST(test_cvar_force_reload_ini_success);
    MU_RUN_TEST(test_cvar_reload_ini_success);
    MU_RUN_TEST(test_cvar_reload_ini_failure_keeps_old);
}

static cvar_type infer_value_type(const char *value) {
    cvar_table table = {0};
    cvar_default_config_parser_handler("test", "value", value, &table);
    cvar_t   *v    = cvar_get(&table, "test.value");
    cvar_type type = v->type;
    cvar_destroy(&table);
    return type;
}

MU_TEST(test_float_suffix_upper_f) { mu_check(infer_value_type("3.14F") == CVAR_FLOAT); }

MU_TEST(test_float_suffix_lower_f) { mu_check(infer_value_type("3.14f") == CVAR_FLOAT); }

MU_TEST(test_float_suffix_one_point_zero) {
    mu_check(infer_value_type("1.0F") == CVAR_FLOAT);
}

MU_TEST(test_float_suffix_zero_point_five) {
    mu_check(infer_value_type("0.5f") == CVAR_FLOAT);
}

MU_TEST(test_float_no_suffix_still_float) {
    mu_check(infer_value_type("3.14") == CVAR_FLOAT);
}

MU_TEST(test_float_suffix_value_correct) {
    cvar_table table = {0};
    mu_check(cvar_default_config_parser_handler("test", "value", "3.14F", &table));
    cvar_t *v = cvar_get(&table, "test.value");
    mu_check(v->value.f > 3.13f && v->value.f < 3.15f);
    cvar_destroy(&table);
}

MU_TEST(test_invalid_dot_suffix_rejected_as_string) {
    mu_check(infer_value_type(".F") == CVAR_STRING);
}

MU_TEST(test_invalid_double_suffix_rejected_as_string) {
    mu_check(infer_value_type("3.14Ff") == CVAR_STRING);
}

MU_TEST(test_invalid_bare_suffix_rejected_as_string) {
    mu_check(infer_value_type("F") == CVAR_STRING);
}

MU_TEST_SUITE(cvar_float_suffix_suite) {
    MU_RUN_TEST(test_float_suffix_upper_f);
    MU_RUN_TEST(test_float_suffix_lower_f);
    MU_RUN_TEST(test_float_suffix_one_point_zero);
    MU_RUN_TEST(test_float_suffix_zero_point_five);
    MU_RUN_TEST(test_float_no_suffix_still_float);
    MU_RUN_TEST(test_float_suffix_value_correct);
    MU_RUN_TEST(test_invalid_dot_suffix_rejected_as_string);
    MU_RUN_TEST(test_invalid_double_suffix_rejected_as_string);
    MU_RUN_TEST(test_invalid_bare_suffix_rejected_as_string);
}

static cvar_t *parse_single(cvar_table *t, const char *value) {
    cvar_default_config_parser_handler("s", "k", value, t);
    return cvar_get(t, "s.k");
}

MU_TEST(test_int_suffix_upper_l) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "42L");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(42, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_lower_l) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "99l");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(99, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_negative) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "-7L");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(-7, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_bare_l_is_string) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "L");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_non_numeric_is_string) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "helloL");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_quote_strip_leading) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "'hello");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("hello", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_quote_strip_does_not_affect_no_quote) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "hello");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("hello", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_quote_strip_int_still_parsed) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "'42");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(42, v->value.i);
    cvar_destroy(&t);
}

MU_TEST_SUITE(cvar_int_suffix_suite) {
    MU_RUN_TEST(test_int_suffix_upper_l);
    MU_RUN_TEST(test_int_suffix_lower_l);
    MU_RUN_TEST(test_int_suffix_negative);
    MU_RUN_TEST(test_int_suffix_bare_l_is_string);
    MU_RUN_TEST(test_int_suffix_non_numeric_is_string);
}

MU_TEST(test_matched_single_quotes_force_string) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "'123123123'");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("123123123", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_matched_double_quotes_force_string) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "\"123123123\"");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("123123123", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_matched_quotes_strip_both) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "'hello world'");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("hello world", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_unmatched_leading_quote_float_suffix) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "'1.23F");
    mu_check(v != NULL && v->type == CVAR_FLOAT);
    mu_check(v->value.f > 1.22f && v->value.f < 1.24f);
    cvar_destroy(&t);
}

MU_TEST(test_unmatched_leading_quote_int_suffix) {
    cvar_table t = {0};
    cvar_t    *v = parse_single(&t, "'123L");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(123, v->value.i);
    cvar_destroy(&t);
}

MU_TEST_SUITE(cvar_quote_strip_suite) {
    MU_RUN_TEST(test_quote_strip_leading);
    MU_RUN_TEST(test_quote_strip_does_not_affect_no_quote);
    MU_RUN_TEST(test_quote_strip_int_still_parsed);
    MU_RUN_TEST(test_matched_single_quotes_force_string);
    MU_RUN_TEST(test_matched_double_quotes_force_string);
    MU_RUN_TEST(test_matched_quotes_strip_both);
    MU_RUN_TEST(test_unmatched_leading_quote_float_suffix);
    MU_RUN_TEST(test_unmatched_leading_quote_int_suffix);
}

int main(void) {
    MU_RUN_SUITE(cvar_reload_suite);
    MU_RUN_SUITE(cvar_float_suffix_suite);
    MU_RUN_SUITE(cvar_int_suffix_suite);
    MU_RUN_SUITE(cvar_quote_strip_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
