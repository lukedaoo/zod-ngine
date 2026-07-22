#define ALL_MODULES_LOG_ENABLED
#define LOG_IMPLEMENTATION
#include "../thirdparty/minunit.h"
#include "modules_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define TEST_SCF "tmp/cvar_typed_test.scf"
#else
#define TEST_SCF "/tmp/cvar_typed_test.scf"
#endif

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    fputs(content, f);
    fclose(f);
}

#define CVAR_IMPLEMENTATION
#include "cvar.h"

#define CVAR_PARSER_IMPLEMENTATION
#include "cvar_parser.h"

#define INI_IMPLEMENTATION
#include "ini.h"

#define SCF_IMPLEMENTATION
#include "scf.h"

#define CVAR_LOAD_IMPLEMENTATION
#include "cvar_load.h"

static bool call_strict(const char *section, const char *key, const char *value,
                        cvar_table *table, const cvar_constraint *entries, size_t count) {
    if (entries) cvar_add_schema(table, entries, count);
    return cvar_strict_handler(section, key, value, table);
}

MU_TEST(test_typed_handler_accepts_correct_int) {
    static const cvar_constraint entries[] = {
         {.name = "window.width", .expected = CVAR_INT}};
    cvar_table table = {0};

    mu_check(call_strict("window", "width", "800", &table, entries, 1));
    mu_assert_int_eq(800, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_rejects_string_for_int) {
    static const cvar_constraint entries[] = {
         {.name = "window.width", .expected = CVAR_INT}};
    cvar_table table = {0};

    mu_check(!call_strict("window", "width", "\"hello\"", &table, entries, 1));
    mu_assert_int_eq(-1, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_rejects_float_for_int) {
    static const cvar_constraint entries[] = {
         {.name = "window.width", .expected = CVAR_INT}};
    cvar_table table = {0};

    mu_check(!call_strict("window", "width", "3.14", &table, entries, 1));
    mu_assert_int_eq(-1, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_coerces_bare_int_for_float) {
    static const cvar_constraint entries[] = {
         {.name = "console.input_gap", .expected = CVAR_FLOAT}};
    cvar_table table = {0};

    mu_check(call_strict("console", "input_gap", "8", &table, entries, 1));
    mu_check(cvar_get_float(&table, "console.input_gap", -1.0f) == 8.0f);

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_accepts_no_matching_entry) {
    static const cvar_constraint entries[] = {
         {.name = "window.width", .expected = CVAR_INT}};
    cvar_table table = {0};

    mu_check(call_strict("window", "height", "600", &table, entries, 1));
    mu_assert_int_eq(600, cvar_get_int(&table, "window.height", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_null_constraints_falls_through) {
    cvar_table table = {0};

    mu_check(call_strict("window", "width", "1280", &table, NULL, 0));
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_accepts_correct_bool) {
    static const cvar_constraint entries[] = {
         {.name = "window.vsync", .expected = CVAR_BOOL}};
    cvar_table table = {0};

    mu_check(call_strict("window", "vsync", "true", &table, entries, 1));
    mu_check(cvar_get_bool(&table, "window.vsync", false) == true);

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_rejects_int_for_bool) {
    static const cvar_constraint entries[] = {
         {.name = "window.vsync", .expected = CVAR_BOOL}};
    cvar_table table = {0};

    mu_check(!call_strict("window", "vsync", "1", &table, entries, 1));
    mu_check(cvar_get_bool(&table, "window.vsync", false) == false);

    cvar_destroy(&table);
}

MU_TEST(test_add_schema_combines_disjoint_entries) {
    static const cvar_constraint a[] = {
         {.name = "window.width", .expected = CVAR_INT},
         {.name = "window.height", .expected = CVAR_INT},
         {.name = "window.vsync", .expected = CVAR_BOOL},
    };
    static const cvar_constraint b[] = {
         {.name = "game.speed", .expected = CVAR_FLOAT},
         {.name = "game.name", .expected = CVAR_STRING},
    };
    cvar_table t = {0};

    mu_check(cvar_add_schema(&t, a, 3));
    mu_check(cvar_add_schema(&t, b, 2));

    mu_check(cvar_find_constraint(&t, "window.width") != NULL);
    mu_check(cvar_find_constraint(&t, "window.height") != NULL);
    mu_check(cvar_find_constraint(&t, "window.vsync") != NULL);
    mu_check(cvar_find_constraint(&t, "game.speed") != NULL);
    mu_check(cvar_find_constraint(&t, "game.name") != NULL);

    cvar_destroy(&t);
}

MU_TEST(test_add_schema_collision_keeps_first_not_silent) {
    static const cvar_constraint a[] = {{.name = "window.width", .expected = CVAR_INT}};
    static const cvar_constraint b[] = {
         {.name = "window.width", .expected = CVAR_STRING}};
    cvar_table t = {0};

    mu_check(cvar_add_schema(&t, a, 1));
    mu_check(!cvar_add_schema(&t, b, 1));

    const cvar_constraint *found = cvar_find_constraint(&t, "window.width");
    mu_check(found != NULL);
    mu_assert_int_eq(CVAR_INT, (int)found->expected);

    cvar_destroy(&t);
}

MU_TEST(test_typed_scf_force_reload_accepts_correct_types) {
    static const cvar_constraint entries[] = {
         {.name = "window.width", .expected = CVAR_INT},
         {.name = "window.height", .expected = CVAR_INT}};
    cvar_table table = {0};
    cvar_set_int(&table, "window.width", 800);
    cvar_set_int(&table, "window.height", 600);
    cvar_add_schema(&table, entries, 2);

    write_file(TEST_SCF, ":/window\nwidth 1280\nheight 720\n");
    mu_check(cvar_load_scf(&table, TEST_SCF, true));
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));
    mu_assert_int_eq(720, cvar_get_int(&table, "window.height", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST(test_typed_scf_force_reload_rejects_type_mismatch) {
    static const cvar_constraint entries[] = {
         {.name = "window.width", .expected = CVAR_INT}};
    cvar_table table = {0};
    cvar_set_int(&table, "window.width", 800);
    cvar_add_schema(&table, entries, 1);

    write_file(TEST_SCF, ":/window\nwidth \"bad\"\n");
    mu_check(!cvar_load_scf(&table, TEST_SCF, true));
    mu_assert_int_eq(800, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST(test_typed_scf_bare_int_for_float_does_not_abort_file) {
    static const cvar_constraint entries[] = {
         {.name = "console.input_gap", .expected = CVAR_FLOAT},
         {.name = "window.width", .expected = CVAR_INT}};
    cvar_table table = {0};
    cvar_set_float(&table, "console.input_gap", 0.0f);
    cvar_set_int(&table, "window.width", 800);
    cvar_add_schema(&table, entries, 2);

    write_file(TEST_SCF, ":/console\ninput_gap 8\n:/window\nwidth 1280\n");
    mu_check(cvar_load_scf(&table, TEST_SCF, true));
    mu_check(cvar_get_float(&table, "console.input_gap", -1.0f) == 8.0f);
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST(test_typed_scf_no_constraints_force_reload_succeeds) {
    cvar_table table = {0};
    cvar_set_int(&table, "window.width", 800);

    write_file(TEST_SCF, ":/window\nwidth 1280\n");
    mu_check(cvar_load_scf(&table, TEST_SCF, true));
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_typed_handler_accepts_correct_int);
    MU_RUN_TEST(test_typed_handler_rejects_string_for_int);
    MU_RUN_TEST(test_typed_handler_rejects_float_for_int);
    MU_RUN_TEST(test_typed_handler_coerces_bare_int_for_float);
    MU_RUN_TEST(test_typed_handler_accepts_no_matching_entry);
    MU_RUN_TEST(test_typed_handler_null_constraints_falls_through);
    MU_RUN_TEST(test_typed_handler_accepts_correct_bool);
    MU_RUN_TEST(test_typed_handler_rejects_int_for_bool);
    MU_RUN_TEST(test_add_schema_combines_disjoint_entries);
    MU_RUN_TEST(test_add_schema_collision_keeps_first_not_silent);
    MU_RUN_TEST(test_typed_scf_force_reload_accepts_correct_types);
    MU_RUN_TEST(test_typed_scf_force_reload_rejects_type_mismatch);
    MU_RUN_TEST(test_typed_scf_bare_int_for_float_does_not_abort_file);
    MU_RUN_TEST(test_typed_scf_no_constraints_force_reload_succeeds);
}

int main(void) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
