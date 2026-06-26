#include "../lib/minunit.h"

#define CARG_IMPLEMENTATION
#include "carg.h"

MU_TEST(test_parse_no_flags) {
    carg_register_t defs[] = {
         {.flag = "--verbose", .arg_count = 0, .type = CARG_BOOL, .required = false}};
    const char *av[]  = {"prog"};
    int         argc  = 1;
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, argc, av, &table));
    mu_check(carg_get_bool(&table, "--verbose", false) == false);

    carg_destroy(&table);
}

MU_TEST(test_parse_bool_flag) {
    carg_register_t defs[] = {
         {.flag = "--verbose", .arg_count = 0, .type = CARG_BOOL, .required = false}};
    const char *av[]  = {"prog", "--verbose"};
    int         argc  = 2;
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, argc, av, &table));
    mu_check(carg_get_bool(&table, "--verbose", false) == true);

    carg_destroy(&table);
}

MU_TEST(test_parse_single_int) {
    carg_register_t defs[] = {
         {.flag = "--count", .arg_count = 1, .type = CARG_INT, .required = false}};

    const char *av[]  = {"prog", "--count", "5"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, 3, av, &table));

    size_t     count = 0;
    const int *vals  = carg_get_int_array(&table, "--count", &count);
    mu_check(count == 1);
    mu_check(vals[0] == 5);

    carg_destroy(&table);
}

MU_TEST(test_parse_single_float) {
    carg_register_t defs[] = {{.flag      = "--master-volume",
                               .arg_count = 1,
                               .type      = CARG_FLOAT,
                               .required  = false}};
    const char     *av[]   = {"prog", "--master-volume", "0.5"};
    carg_table      table  = {0};

    mu_check(carg_parse(defs, 1, 3, av, &table));

    size_t       count = 0;
    const float *vals  = carg_get_float_array(&table, "--master-volume", &count);
    mu_check(count == 1);
    mu_check(vals[0] == 0.5F);

    carg_destroy(&table);
}

MU_TEST(test_parse_single_string) {
    carg_register_t defs[] = {
         {.flag = "--name", .arg_count = 1, .type = CARG_STRING, .required = false}};
    const char *av[]  = {"prog", "--name", "test"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, 3, av, &table));

    size_t       count = 0;
    const char **vals  = carg_get_string_array(&table, "--name", &count);
    mu_check(count == 1);
    mu_assert_string_eq("test", vals[0]);

    carg_destroy(&table);
}

MU_TEST(test_parse_multiple_ints) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog", "--size", "10", "20"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, 4, av, &table));

    size_t     count = 0;
    const int *vals  = carg_get_int_array(&table, "--size", &count);
    mu_check(count == 2);
    mu_check(vals[0] == 10);
    mu_check(vals[1] == 20);

    carg_destroy(&table);
}

MU_TEST(test_parse_multiple_floats) {
    carg_register_t defs[] = {
         {.flag = "--range", .arg_count = 2, .type = CARG_FLOAT, .required = false}};
    const char *av[]  = {"prog", "--range", "1.5", "2.5"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, 4, av, &table));

    size_t       count = 0;
    const float *vals  = carg_get_float_array(&table, "--range", &count);
    mu_check(count == 2);
    mu_check(vals[0] == 1.5F);
    mu_check(vals[1] == 2.5F);

    carg_destroy(&table);
}

MU_TEST(test_parse_multiple_strings) {
    carg_register_t defs[] = {
         {.flag = "--tags", .arg_count = 2, .type = CARG_STRING, .required = false}};
    const char *av[]  = {"prog", "--tags", "foo", "bar"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, 4, av, &table));

    size_t       count = 0;
    const char **vals  = carg_get_string_array(&table, "--tags", &count);
    mu_check(count == 2);
    mu_assert_string_eq("foo", vals[0]);
    mu_assert_string_eq("bar", vals[1]);

    carg_destroy(&table);
}

MU_TEST(test_parse_required_and_optional_mix) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 1, .type = CARG_INT, .required = true},
         {.flag = "--verbose", .arg_count = 0, .type = CARG_BOOL, .required = false},
    };
    const char *av[]  = {"prog", "--size", "5"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 2, 3, av, &table));
    mu_check(carg_get_bool(&table, "--verbose", false) == false);

    size_t     count = 0;
    const int *vals  = carg_get_int_array(&table, "--size", &count);
    mu_check(count == 1);
    mu_check(vals[0] == 5);

    carg_destroy(&table);
}

MU_TEST(test_parse_bad_flag_format_fails) {
    carg_register_t single_dash[] = {
         {.flag = "-size", .arg_count = 1, .type = CARG_INT, .required = false}};
    carg_register_t no_dash[] = {
         {.flag = "size", .arg_count = 1, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog"};
    carg_table  table = {0};

    mu_check(!carg_parse(single_dash, 1, 1, av, &table));
    mu_check(!carg_parse(no_dash, 1, 1, av, &table));
}

MU_TEST(test_parse_unknown_flag_fails) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 1, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog", "--bogus", "5"};
    carg_table  table = {0};

    mu_check(!carg_parse(defs, 1, 3, av, &table));
}

MU_TEST(test_parse_missing_required_fails) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 1, .type = CARG_INT, .required = true}};
    const char *av[]  = {"prog"};
    carg_table  table = {0};

    mu_check(!carg_parse(defs, 1, 1, av, &table));
}

MU_TEST(test_parse_too_few_values_fails) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog", "--size", "10"};
    carg_table  table = {0};

    mu_check(!carg_parse(defs, 1, 3, av, &table));
}

MU_TEST(test_parse_invalid_int_fails) {
    carg_register_t defs[] = {
         {.flag = "--count", .arg_count = 1, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog", "--count", "abc"};
    carg_table  table = {0};

    mu_check(!carg_parse(defs, 1, 3, av, &table));
}

MU_TEST(test_parse_invalid_float_fails) {
    carg_register_t defs[] = {{.flag      = "--master-volume",
                               .arg_count = 1,
                               .type      = CARG_FLOAT,
                               .required  = false}};
    const char     *av[]   = {"prog", "--master-volume", "abc"};
    carg_table      table  = {0};

    mu_check(!carg_parse(defs, 1, 3, av, &table));
}

MU_TEST(test_getter_bool_present_and_absent) {
    carg_register_t defs[] = {
         {.flag = "--verbose", .arg_count = 0, .type = CARG_BOOL, .required = false},
         {.flag = "--quiet", .arg_count = 0, .type = CARG_BOOL, .required = false},
    };
    const char *av[]  = {"prog", "--verbose"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 2, 2, av, &table));
    mu_check(carg_get_bool(&table, "--verbose", false) == true);
    mu_check(carg_get_bool(&table, "--quiet", false) == false);
    mu_check(carg_get_bool(&table, "--nonexistent", true) == true);

    carg_destroy(&table);
}

MU_TEST(test_getter_int_array_present_and_absent) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 1, .type = CARG_INT, .required = false},
         {.flag = "--scale", .arg_count = 1, .type = CARG_INT, .required = false},
    };
    const char *av[]  = {"prog", "--size", "7"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 2, 3, av, &table));

    size_t count = 0;
    mu_check(carg_get_int_array(&table, "--size", &count) != NULL);
    mu_check(count == 1);

    count = 99;
    mu_check(carg_get_int_array(&table, "--scale", &count) == NULL);
    mu_check(count == 0);

    carg_destroy(&table);
}

MU_TEST(test_getter_float_array_present_and_absent) {
    carg_register_t defs[] = {
         {.flag = "--volume", .arg_count = 1, .type = CARG_FLOAT, .required = false},
         {.flag = "--gain", .arg_count = 1, .type = CARG_FLOAT, .required = false},
    };
    const char *av[]  = {"prog", "--volume", "0.5"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 2, 3, av, &table));

    size_t count = 0;
    mu_check(carg_get_float_array(&table, "--volume", &count) != NULL);
    mu_check(count == 1);

    count = 99;
    mu_check(carg_get_float_array(&table, "--gain", &count) == NULL);
    mu_check(count == 0);

    carg_destroy(&table);
}

MU_TEST(test_getter_string_array_present_and_absent) {
    carg_register_t defs[] = {
         {.flag = "--name", .arg_count = 1, .type = CARG_STRING, .required = false},
         {.flag = "--title", .arg_count = 1, .type = CARG_STRING, .required = false},
    };
    const char *av[]  = {"prog", "--name", "test"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 2, 3, av, &table));

    size_t count = 0;
    mu_check(carg_get_string_array(&table, "--name", &count) != NULL);
    mu_check(count == 1);

    count = 99;
    mu_check(carg_get_string_array(&table, "--title", &count) == NULL);
    mu_check(count == 0);

    carg_destroy(&table);
}

MU_TEST(test_get_returns_null_for_nonexistent_flag) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 1, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog", "--size", "5"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 1, 3, av, &table));
    mu_check(carg_get(&table, "--nonexistent") == NULL);

    carg_destroy(&table);
}

MU_TEST(test_parse_excess_values_fails) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false}};
    const char *av[]  = {"prog", "--size", "10", "20", "300"};
    carg_table  table = {0};

    mu_check(!carg_parse(defs, 1, 5, av, &table));
}

MU_TEST(test_parse_excess_values_before_flag_fails) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
         {.flag = "--verbose", .arg_count = 0, .type = CARG_BOOL, .required = false},
    };
    const char *av[]  = {"prog", "--size", "10", "20", "300", "--verbose"};
    carg_table  table = {0};

    mu_check(!carg_parse(defs, 2, 6, av, &table));
}

MU_TEST(test_destroy_null_safe) { carg_destroy(NULL); }

MU_TEST(test_destroy_populated_table_all_types) {
    carg_register_t defs[] = {
         {.flag = "--size", .arg_count = 1, .type = CARG_INT, .required = false},
         {.flag = "--volume", .arg_count = 1, .type = CARG_FLOAT, .required = false},
         {.flag = "--name", .arg_count = 1, .type = CARG_STRING, .required = false},
         {.flag = "--verbose", .arg_count = 0, .type = CARG_BOOL, .required = false},
    };
    const char *av[]  = {"prog", "--size", "5",    "--volume",
                         "0.5",  "--name", "test", "--verbose"};
    carg_table  table = {0};

    mu_check(carg_parse(defs, 4, 8, av, &table));

    carg_destroy(&table);
}

MU_TEST_SUITE(carg_suite) {
    MU_RUN_TEST(test_parse_no_flags);
    MU_RUN_TEST(test_parse_bool_flag);
    MU_RUN_TEST(test_parse_single_int);
    MU_RUN_TEST(test_parse_single_float);
    MU_RUN_TEST(test_parse_single_string);
    MU_RUN_TEST(test_parse_multiple_ints);
    MU_RUN_TEST(test_parse_multiple_floats);
    MU_RUN_TEST(test_parse_multiple_strings);
    MU_RUN_TEST(test_parse_required_and_optional_mix);
    MU_RUN_TEST(test_parse_bad_flag_format_fails);
    MU_RUN_TEST(test_parse_unknown_flag_fails);
    MU_RUN_TEST(test_parse_missing_required_fails);
    MU_RUN_TEST(test_parse_too_few_values_fails);
    MU_RUN_TEST(test_parse_invalid_int_fails);
    MU_RUN_TEST(test_parse_invalid_float_fails);
    MU_RUN_TEST(test_getter_bool_present_and_absent);
    MU_RUN_TEST(test_getter_int_array_present_and_absent);
    MU_RUN_TEST(test_getter_float_array_present_and_absent);
    MU_RUN_TEST(test_getter_string_array_present_and_absent);
    MU_RUN_TEST(test_get_returns_null_for_nonexistent_flag);
    MU_RUN_TEST(test_parse_excess_values_fails);
    MU_RUN_TEST(test_parse_excess_values_before_flag_fails);
    MU_RUN_TEST(test_destroy_null_safe);
    MU_RUN_TEST(test_destroy_populated_table_all_types);
}

int main(void) {
    MU_RUN_SUITE(carg_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
