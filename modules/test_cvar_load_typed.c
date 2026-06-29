#define MODULE_LOG_ENABLED
#include "../lib/minunit.h"

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

#define INI_IMPLEMENTATION
#include "ini.h"

#define SCF_IMPLEMENTATION
#include "scf.h"

#define CVAR_LOAD_IMPLEMENTATION
#include "cvar_load.h"

static bool call_strict(const char *section, const char *key, const char *value,
                        cvar_table *table, const cvar_schema *schema) {
    cvar_load_ctx ctx = {.table = table, .schema = schema};
    return cvar_strict_handler(section, key, value, &ctx);
}

MU_TEST(test_typed_handler_accepts_correct_int) {
    static const cvar_schema_entry entries[] = {{"window.width", CVAR_INT}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};

    mu_check(call_strict("window", "width", "800", &table, &schema));
    mu_assert_int_eq(800, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_rejects_string_for_int) {
    static const cvar_schema_entry entries[] = {{"window.width", CVAR_INT}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};

    mu_check(!call_strict("window", "width", "\"hello\"", &table, &schema));
    mu_assert_int_eq(-1, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_rejects_float_for_int) {
    static const cvar_schema_entry entries[] = {{"window.width", CVAR_INT}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};

    mu_check(!call_strict("window", "width", "3.14", &table, &schema));
    mu_assert_int_eq(-1, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_accepts_no_schema_entry) {
    static const cvar_schema_entry entries[] = {{"window.width", CVAR_INT}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};

    mu_check(call_strict("window", "height", "600", &table, &schema));
    mu_assert_int_eq(600, cvar_get_int(&table, "window.height", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_null_schema_falls_through) {
    cvar_table table = {0};

    mu_check(call_strict("window", "width", "1280", &table, NULL));
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_accepts_correct_bool) {
    static const cvar_schema_entry entries[] = {{"window.vsync", CVAR_BOOL}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};

    mu_check(call_strict("window", "vsync", "true", &table, &schema));
    mu_check(cvar_get_bool(&table, "window.vsync", false) == true);

    cvar_destroy(&table);
}

MU_TEST(test_typed_handler_rejects_int_for_bool) {
    static const cvar_schema_entry entries[] = {{"window.vsync", CVAR_BOOL}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};

    mu_check(!call_strict("window", "vsync", "1", &table, &schema));
    mu_check(cvar_get_bool(&table, "window.vsync", false) == false);

    cvar_destroy(&table);
}

MU_TEST(test_schema_merge_combines_entries) {
    static const cvar_schema_entry a[] = {
         {"window.width", CVAR_INT},
         {"window.height", CVAR_INT},
         {"window.vsync", CVAR_BOOL},
    };
    static const cvar_schema_entry b[] = {
         {"game.speed", CVAR_FLOAT},
         {"game.name", CVAR_STRING},
    };
    static const cvar_schema sa = {.entries = a, .count = 3};
    static const cvar_schema sb = {.entries = b, .count = 2};

    cvar_schema_entry out[8];
    size_t            n = cvar_schema_merge(&sa, &sb, out, 8);

    mu_assert_int_eq(5, (int)n);
}

MU_TEST(test_schema_merge_second_overrides_first) {
    static const cvar_schema_entry a[] = {{"window.width", CVAR_INT}};
    static const cvar_schema_entry b[] = {{"window.width", CVAR_STRING}};
    static const cvar_schema       sa  = {.entries = a, .count = 1};
    static const cvar_schema       sb  = {.entries = b, .count = 1};

    cvar_schema_entry out[4];
    size_t            n = cvar_schema_merge(&sa, &sb, out, 4);

    mu_assert_int_eq(1, (int)n);
    mu_assert_int_eq(CVAR_STRING, (int)out[0].expected);
}

MU_TEST(test_typed_scf_force_reload_accepts_correct_types) {
    static const cvar_schema_entry entries[] = {{"window.width", CVAR_INT},
                                                {"window.height", CVAR_INT}};
    static const cvar_schema       schema    = {.entries = entries, .count = 2};
    cvar_table                     table     = {0};
    cvar_set_int(&table, "window.width", 800);
    cvar_set_int(&table, "window.height", 600);

    write_file(TEST_SCF, ":/window\nwidth 1280\nheight 720\n");
    mu_check(cvar_load_scf(&table, TEST_SCF, &schema, true));
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));
    mu_assert_int_eq(720, cvar_get_int(&table, "window.height", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST(test_typed_scf_force_reload_rejects_type_mismatch) {
    static const cvar_schema_entry entries[] = {{"window.width", CVAR_INT}};
    static const cvar_schema       schema    = {.entries = entries, .count = 1};
    cvar_table                     table     = {0};
    cvar_set_int(&table, "window.width", 800);

    write_file(TEST_SCF, ":/window\nwidth \"bad\"\n");
    mu_check(!cvar_load_scf(&table, TEST_SCF, &schema, true));
    mu_assert_int_eq(800, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST(test_typed_scf_null_schema_force_reload_succeeds) {
    cvar_table table = {0};
    cvar_set_int(&table, "window.width", 800);

    write_file(TEST_SCF, ":/window\nwidth 1280\n");
    mu_check(cvar_load_scf(&table, TEST_SCF, NULL, true));
    mu_assert_int_eq(1280, cvar_get_int(&table, "window.width", -1));

    cvar_destroy(&table);
    remove(TEST_SCF);
}

MU_TEST_SUITE(test_suite) {
    MU_RUN_TEST(test_typed_handler_accepts_correct_int);
    MU_RUN_TEST(test_typed_handler_rejects_string_for_int);
    MU_RUN_TEST(test_typed_handler_rejects_float_for_int);
    MU_RUN_TEST(test_typed_handler_accepts_no_schema_entry);
    MU_RUN_TEST(test_typed_handler_null_schema_falls_through);
    MU_RUN_TEST(test_typed_handler_accepts_correct_bool);
    MU_RUN_TEST(test_typed_handler_rejects_int_for_bool);
    MU_RUN_TEST(test_schema_merge_combines_entries);
    MU_RUN_TEST(test_schema_merge_second_overrides_first);
    MU_RUN_TEST(test_typed_scf_force_reload_accepts_correct_types);
    MU_RUN_TEST(test_typed_scf_force_reload_rejects_type_mismatch);
    MU_RUN_TEST(test_typed_scf_null_schema_force_reload_succeeds);
}

int main(void) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
