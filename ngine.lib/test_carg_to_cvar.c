#define CARG_IMPLEMENTATION
#define CVAR_IMPLEMENTATION
#define LOG_IMPLEMENTATION

#include "../thirdparty/minunit.h"
#include "carg.h"
#include "carg_to_cvar.h"
#include "cvar.h"

MU_TEST(test_int_conversion) {
    int         values[] = {200, 300};
    carg        carg     = {.flag    = "--size",
                            .type    = CARG_INT,
                            .count   = 2,
                            .value.i = values,
                            .present = true};
    const char *names[]  = {"width", "height"};
    cvar_table  table    = {0};

    mu_check(carg_entry_to_cvars(&carg, names, 2, &table));
    mu_assert_int_eq(200, cvar_get_int(&table, "width", -1));
    mu_assert_int_eq(300, cvar_get_int(&table, "height", -1));

    cvar_destroy(&table);
}

MU_TEST(test_int_conversion_when_value_is_not_present) {
    carg        carg    = {.flag    = "--size",
                           .type    = CARG_INT,
                           .count   = 2,
                           .value.i = NULL,
                           .present = false};
    const char *names[] = {"width", "height"};
    cvar_table  table   = {0};

    mu_check(carg_entry_to_cvars(&carg, names, 2, &table));
    mu_assert_int_eq(-1, cvar_get_int(&table, "width", -1));
    mu_assert_int_eq(-1, cvar_get_int(&table, "height", -1));

    cvar_destroy(&table);
}

MU_TEST(test_float_conversion) {
    float       values[] = {1.5f, 2.5f};
    carg        carg     = {.flag    = "--scale",
                            .type    = CARG_FLOAT,
                            .count   = 2,
                            .value.f = values,
                            .present = true};
    const char *names[]  = {"sx", "sy"};
    cvar_table  table    = {0};

    mu_check(carg_entry_to_cvars(&carg, names, 2, &table));
    mu_check(cvar_get_float(&table, "sx", -1.0f) == 1.5f);
    mu_check(cvar_get_float(&table, "sy", -1.0f) == 2.5f);

    cvar_destroy(&table);
}

MU_TEST(test_string_conversion) {
    const char *values[] = {"a.ini", "b.ini"};
    carg        carg     = {.flag    = "--paths",
                            .type    = CARG_STRING,
                            .count   = 2,
                            .value.s = values,
                            .present = true};
    const char *names[]  = {"path1", "path2"};
    cvar_table  table    = {0};

    mu_check(carg_entry_to_cvars(&carg, names, 2, &table));
    mu_assert_string_eq("a.ini", cvar_get_string(&table, "path1", NULL));
    mu_assert_string_eq("b.ini", cvar_get_string(&table, "path2", NULL));

    cvar_destroy(&table);
}

MU_TEST(test_bool_present_maps_true) {
    carg        carg    = {.flag    = "--verbose",
                           .type    = CARG_BOOL,
                           .present = true,
                           .count   = 0,
                           .value.b = true};
    const char *names[] = {"verbose"};
    cvar_table  table   = {0};

    mu_check(carg_entry_to_cvars(&carg, names, 1, &table));
    mu_check(cvar_get_bool(&table, "verbose", false) == true);

    cvar_destroy(&table);
}

MU_TEST(test_bool_absent_skips) {
    carg carg = {.flag = "--verbose", .type = CARG_BOOL, .present = false, .count = 0};
    const char *names[] = {"verbose"};
    cvar_table  table   = {0};

    mu_check(carg_entry_to_cvars(&carg, names, 1, &table));
    mu_check(cvar_get_bool(&table, "verbose", true) == true);  // fallback, not written

    cvar_destroy(&table);
}

MU_TEST(test_bool_wrong_names_count_rejected) {
    carg carg = {.flag = "--verbose", .type = CARG_BOOL, .present = true, .count = 0};
    const char *names[] = {"a", "b"};
    cvar_table  table   = {0};

    mu_check(!carg_entry_to_cvars(&carg, names, 2, &table));
    mu_check(table.size == 0);

    cvar_destroy(&table);
}

MU_TEST(test_names_count_mismatch_rejected) {
    int         values[] = {200, 300};
    carg        carg     = {.flag    = "--size",
                            .type    = CARG_INT,
                            .count   = 2,
                            .present = true,
                            .value.i = values};
    const char *names[]  = {"width"};
    cvar_table  table    = {0};

    mu_check(!carg_entry_to_cvars(&carg, names, 1, &table));
    mu_check(table.size == 0);

    cvar_destroy(&table);
}

MU_TEST(test_null_args_rejected) {
    int  values[] = {200};
    carg carg     = {.flag = "--size", .type = CARG_INT, .count = 1, .value.i = values};
    const char *names[] = {"width"};
    cvar_table  table   = {0};

    mu_check(!carg_entry_to_cvars(NULL, names, 1, &table));
    mu_check(!carg_entry_to_cvars(&carg, NULL, 1, &table));
    mu_check(!carg_entry_to_cvars(&carg, names, 1, NULL));
}

MU_TEST(test_table_conversion) {
    int  size_values[] = {200, 300};
    carg entries[2]    = {
         {.flag    = "--size",
          .type    = CARG_INT,
          .count   = 2,
          .value.i = size_values,
          .present = true},
         {.flag    = "--verbose",
          .type    = CARG_BOOL,
          .present = true,
          .count   = 0,
          .value.b = true},
    };
    carg_table cargs = {.data = entries, .size = 2};

    const char *size_names[]    = {"width", "height"};
    const char *verbose_names[] = {"verbose"};

    const char **names_per_carg[]       = {size_names, verbose_names};
    const size_t names_count_per_carg[] = {2, 1};

    cvar_table table = {0};

    mu_check(carg_table_to_cvars(&cargs, names_per_carg, names_count_per_carg, &table));
    mu_assert_int_eq(200, cvar_get_int(&table, "width", -1));
    mu_assert_int_eq(300, cvar_get_int(&table, "height", -1));
    mu_check(cvar_get_bool(&table, "verbose", false) == true);

    cvar_destroy(&table);
}

MU_TEST(test_table_conversion_skipping_on_first_failure) {
    int  size_values[] = {200, 300};
    carg entries[2]    = {
         {.flag = "--size", .type = CARG_INT, .count = 2, .value.i = size_values},
         {.flag = "--verbose", .type = CARG_BOOL, .present = true, .count = 0},
    };
    carg_table cargs = {.data = entries, .size = 2};

    const char *size_names_wrong[] = {"width"};
    const char *verbose_names[]    = {"verbose"};

    const char **names_per_carg[]       = {size_names_wrong, verbose_names};
    const size_t names_count_per_carg[] = {1, 1};

    cvar_table table = {0};

    mu_check(carg_table_to_cvars(&cargs, names_per_carg, names_count_per_carg, &table));

    cvar_destroy(&table);
}

MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_int_conversion);
    MU_RUN_TEST(test_int_conversion_when_value_is_not_present);
    MU_RUN_TEST(test_float_conversion);
    MU_RUN_TEST(test_string_conversion);
    MU_RUN_TEST(test_bool_present_maps_true);
    MU_RUN_TEST(test_bool_absent_skips);
    MU_RUN_TEST(test_bool_wrong_names_count_rejected);
    MU_RUN_TEST(test_names_count_mismatch_rejected);
    MU_RUN_TEST(test_null_args_rejected);
    MU_RUN_TEST(test_table_conversion);
    MU_RUN_TEST(test_table_conversion_skipping_on_first_failure);
}

int main(void) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
