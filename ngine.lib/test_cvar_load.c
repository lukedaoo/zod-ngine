#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_IMPLEMENTATION
#include "log.h"

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

MU_TEST(test_cvar_force_reload_ini_success) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(cvar_load_ini(&table, TEST_INI, false));

    write_file(TEST_INI, "[test]\nvalue=2\nname=bob\n");
    mu_check(cvar_load_ini(&table, TEST_INI, true));

    cvar *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 2);

    cvar *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("bob", n->value.str.data);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST(test_cvar_reload_ini_success) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(cvar_load_ini(&table, TEST_INI, false));

    write_file(TEST_INI, "[test]\nvalue=2\nname=bob\n");
    mu_check(cvar_load_ini(&table, TEST_INI, false));

    cvar *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 2);

    cvar *n = cvar_get(&table, "test.name");
    mu_assert_string_eq("bob", n->value.str.data);

    cvar_destroy(&table);
    remove(TEST_INI);
}

MU_TEST(test_cvar_reload_ini_failure_keeps_old) {
    cvar_table table = {0};

    write_file(TEST_INI, "[test]\nvalue=1\nname=alice\n");
    mu_check(cvar_load_ini(&table, TEST_INI, false));

    static const cvar_constraint entries[] = {
         {.name = "test.value", .expected = CVAR_BOOL}};
    cvar_add_schema(&table, entries, 1);

    // value=99 infers CVAR_INT; constraint expects CVAR_BOOL → mismatch → fail
    write_file(TEST_INI, "[test]\nvalue=99\nname=bob\n");
    mu_check(!cvar_load_ini(&table, TEST_INI, true));

    cvar *v = cvar_get(&table, "test.value");
    mu_check(v->value.i == 1);

    cvar *n = cvar_get(&table, "test.name");
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
    cvar_parse_and_set("test", "value", value, &table);
    cvar     *v    = cvar_get(&table, "test.value");
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
    mu_check(cvar_parse_and_set("test", "value", "3.14F", &table));
    cvar *v = cvar_get(&table, "test.value");
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

static cvar *parse_single(cvar_table *t, const char *value) {
    cvar_parse_and_set("s", "k", value, t);
    return cvar_get(t, "s.k");
}

MU_TEST(test_int_suffix_upper_l) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "42L");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(42, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_lower_l) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "99l");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(99, v->value.i);

    cvar *_v = parse_single(&t, "'99l");
    mu_check(_v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(99, _v->value.i);

    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_negative) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "-7L");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(-7, v->value.i);

    cvar *_v = parse_single(&t, "'-7L");
    mu_check(_v != NULL && _v->type == CVAR_INT);
    mu_assert_int_eq(-7, _v->value.i);

    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_bare_l_is_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "L");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_non_numeric_is_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "helloL");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_quote_strip_leading) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "'hello");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("hello", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_quote_strip_does_not_affect_no_quote) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "hello");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("hello", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_quote_strip_int_still_parsed) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "'42");
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
    cvar      *v = parse_single(&t, "'123123123'");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("123123123", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_matched_double_quotes_force_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "\"123123123\"");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("123123123", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_matched_quotes_strip_both) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "'hello world'");
    mu_check(v != NULL && v->type == CVAR_STRING);
    mu_assert_string_eq("hello world", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_unmatched_leading_quote_float_suffix) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "'1.23F");
    mu_check(v != NULL && v->type == CVAR_FLOAT);
    mu_check(v->value.f > 1.22f && v->value.f < 1.24f);
    cvar_destroy(&t);
}

MU_TEST(test_unmatched_leading_quote_int_suffix) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "'123L");
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

MU_TEST(test_hex_zero) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0x0");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(0, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_one) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0x1");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(1, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_ten) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0x10");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(16, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_ff_lower) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xff");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(255, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_ff_upper) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xFF");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(255, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_uppercase_x_prefix) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0Xff");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(255, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_rgb_white) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xFFFFFF");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(16777215, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_rgba_all_ones_bit_preserved) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xFFFFFFFF");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_assert_int_eq(-1, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_only_prefix_is_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0x");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_hex_invalid_digits_is_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xGG");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_hex_trailing_invalid_is_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0x10g");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

MU_TEST(test_hex_no_prefix_is_string) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "ff");
    mu_check(v != NULL && v->type == CVAR_STRING);
    cvar_destroy(&t);
}

// Regression: hex values ending in 'f'/'F' were matched by the float-suffix
// check (strtof parses "0xf" as 15.0 on glibc), producing CVAR_FLOAT instead
// of CVAR_INT. The is_hex guard in the suffix block fixes this.
MU_TEST(test_hex_ending_f_not_float_beef) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xbeef");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_check(v->type != CVAR_FLOAT);
    mu_assert_int_eq(0xbeef, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_ending_F_not_float_BEEF) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xBEEF");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_check(v->type != CVAR_FLOAT);
    mu_assert_int_eq(0xBEEF, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_hex_ending_f_not_float_deadbeef) {
    cvar_table t = {0};
    cvar      *v = parse_single(&t, "0xdeadbeef");
    mu_check(v != NULL && v->type == CVAR_INT);
    mu_check(v->type != CVAR_FLOAT);
    mu_assert_int_eq((int)0xdeadbeef, v->value.i);
    cvar_destroy(&t);
}

MU_TEST_SUITE(cvar_hex_suite) {
    MU_RUN_TEST(test_hex_zero);
    MU_RUN_TEST(test_hex_one);
    MU_RUN_TEST(test_hex_ten);
    MU_RUN_TEST(test_hex_ff_lower);
    MU_RUN_TEST(test_hex_ff_upper);
    MU_RUN_TEST(test_hex_uppercase_x_prefix);
    MU_RUN_TEST(test_hex_rgb_white);
    MU_RUN_TEST(test_hex_rgba_all_ones_bit_preserved);
    MU_RUN_TEST(test_hex_ending_f_not_float_beef);
    MU_RUN_TEST(test_hex_ending_F_not_float_BEEF);
    MU_RUN_TEST(test_hex_ending_f_not_float_deadbeef);
    MU_RUN_TEST(test_hex_only_prefix_is_string);
    MU_RUN_TEST(test_hex_invalid_digits_is_string);
    MU_RUN_TEST(test_hex_trailing_invalid_is_string);
    MU_RUN_TEST(test_hex_no_prefix_is_string);
}

static inline cvar *parse_entry(cvar_table *table, const cvar_constraint *entry,
                                const char *section, const char *key, const char *value,
                                bool *ok_out) {
    if (entry) cvar_add_schema(table, entry, 1);

    char full[CVAR_NAME_MAX];
    snprintf(full, sizeof(full), "%s.%s", section, key);

    bool ok = cvar_parse_and_set(section, key, value, table);
    if (ok_out) *ok_out = ok;

    return cvar_get(table, full);
}

static inline cvar_constraint entry_int(const char *name) {
    return (cvar_constraint){.name = name, .expected = CVAR_INT};
}

static inline cvar_constraint entry_int_range(const char *name, int min, int max) {
    return (cvar_constraint){
         .name     = name,
         .expected = CVAR_INT,
         .range    = {.has_min = true, .min.i = min, .has_max = true, .max.i = max},
    };
}

static inline cvar_constraint entry_float(const char *name) {
    return (cvar_constraint){.name = name, .expected = CVAR_FLOAT};
}

static inline cvar_constraint entry_float_range(const char *name, float min, float max) {
    return (cvar_constraint){
         .name     = name,
         .expected = CVAR_FLOAT,
         .range    = {.has_min = true, .min.f = min, .has_max = true, .max.f = max},
    };
}

static inline cvar_constraint entry_bool(const char *name) {
    return (cvar_constraint){.name = name, .expected = CVAR_BOOL};
}

static inline cvar_constraint entry_string(const char *name) {
    return (cvar_constraint){.name = name, .expected = CVAR_STRING};
}

MU_TEST(test_int_in_range) {
    cvar_constraint e = entry_int_range("test.int", 0, 100);
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "int", "42", &ok);

    mu_check(ok);
    mu_assert_int_eq(42, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_int_below_range) {
    cvar_constraint e = entry_int_range("test.int", 0, 100);
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "int", "-12", &ok);

    mu_check(!ok);
    mu_check(v == NULL);
    cvar_destroy(&t);
}

MU_TEST(test_int_above_range) {
    cvar_constraint e = entry_int_range("test.int", 0, 100);
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "int", "999", &ok);

    mu_check(!ok);
    mu_check(v == NULL);
    cvar_destroy(&t);
}

MU_TEST(test_int_suffix_l) {
    cvar_constraint e = entry_int("test.int");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "int", "123l", &ok);

    mu_check(ok);
    mu_assert_int_eq(123, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_int_hex) {
    cvar_constraint e = entry_int("test.hex");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "hex", "0xFF", &ok);

    mu_check(ok);
    mu_assert_int_eq(255, v->value.i);
    cvar_destroy(&t);
}

MU_TEST(test_float_in_range) {
    cvar_constraint e = entry_float_range("test.float", 0.f, 10.f);
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "float", "3.14", &ok);

    mu_check(ok);
    mu_assert_double_eq(3.14f, v->value.f);
    cvar_destroy(&t);
}

MU_TEST(test_float_suffix_f) {
    cvar_constraint e = entry_float("test.float");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "float", "1.5f", &ok);

    mu_check(ok);
    mu_assert_double_eq(1.5f, v->value.f);
    cvar_destroy(&t);
}

MU_TEST(test_float_out_of_range) {
    cvar_constraint e = entry_float_range("test.float", 0.f, 1.f);
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "float", "5.0", &ok);

    mu_check(!ok);
    mu_check(v == NULL);
    cvar_destroy(&t);
}

MU_TEST(test_bool_true) {
    cvar_constraint e = entry_bool("test.bool");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "bool", "true", &ok);

    mu_check(ok);
    mu_check(v->value.b == true);
    cvar_destroy(&t);
}

MU_TEST(test_bool_false) {
    cvar_constraint e = entry_bool("test.bool");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "bool", "false", &ok);

    mu_check(ok);
    mu_check(v->value.b == false);
    cvar_destroy(&t);
}

MU_TEST(test_string_quoted) {
    cvar_constraint e = entry_string("test.str");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "str", "\"hello\"", &ok);

    mu_check(ok);
    mu_assert_string_eq("hello", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST(test_string_unquoted) {
    cvar_constraint e = entry_string("test.str");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "str", "world", &ok);

    mu_check(ok);
    mu_assert_string_eq("world", v->value.str.data);
    cvar_destroy(&t);
}

// A bare int literal for a float-declared cvar is coerced, not rejected —
// otherwise a config author writing "8" instead of "8.0" would abort the
// whole file's load (ini_parse/scf_parse stop on the first failing line).
MU_TEST(test_bare_int_coerced_for_float) {
    cvar_constraint e = entry_float("test.mismatch");
    cvar_table      t = {0};
    bool            ok;
    cvar           *v = parse_entry(&t, &e, "test", "mismatch", "123", &ok);

    mu_check(ok);
    mu_check(v != NULL);
    mu_check(v->type == CVAR_FLOAT);
    mu_check(v->value.f == 123.0f);
    cvar_destroy(&t);
}

MU_TEST(test_no_constraints_allows_anything) {
    cvar_table t = {0};
    bool       ok;
    cvar      *v = parse_entry(&t, NULL, "test", "free", "123", &ok);

    mu_check(ok);
    mu_assert_int_eq(123, v->value.i);
    cvar_destroy(&t);
}

// Regression: an earlier version of cvar_parse synthesized a same-type constraint
// whenever no schema was registered, locking a schema-less cvar to whatever type it
// was first seeded with. That broke config.c's "log.level", which is seeded as
// CVAR_INT but the config file legitimately overrides it with a string level name
// ("debug"), converted back to int by config_priv_adjust() after load — a schema-less
// cvar's type must stay free to change on every parse, not just its first one.
MU_TEST(test_no_constraints_allows_existing_cvar_type_to_change) {
    cvar_table t = {0};
    cvar_set_int(&t, "log.level", 0);

    bool  ok;
    cvar *v = parse_entry(&t, NULL, "log", "level", "\"debug\"", &ok);

    mu_check(ok);
    mu_check(v->type == CVAR_STRING);
    mu_assert_string_eq("debug", v->value.str.data);
    cvar_destroy(&t);
}

MU_TEST_SUITE(cvar_parse_with_range_suite) {
    MU_RUN_TEST(test_int_in_range);
    MU_RUN_TEST(test_int_below_range);
    MU_RUN_TEST(test_int_above_range);
    MU_RUN_TEST(test_int_suffix_l);
    MU_RUN_TEST(test_int_hex);

    MU_RUN_TEST(test_float_in_range);
    MU_RUN_TEST(test_float_suffix_f);
    MU_RUN_TEST(test_float_out_of_range);

    MU_RUN_TEST(test_bool_true);
    MU_RUN_TEST(test_bool_false);

    MU_RUN_TEST(test_string_quoted);
    MU_RUN_TEST(test_string_unquoted);

    MU_RUN_TEST(test_bare_int_coerced_for_float);

    MU_RUN_TEST(test_no_constraints_allows_anything);
    MU_RUN_TEST(test_no_constraints_allows_existing_cvar_type_to_change);
}

int main(void) {
    MU_RUN_SUITE(cvar_reload_suite);
    MU_RUN_SUITE(cvar_float_suffix_suite);
    MU_RUN_SUITE(cvar_int_suffix_suite);
    MU_RUN_SUITE(cvar_quote_strip_suite);
    MU_RUN_SUITE(cvar_hex_suite);
    MU_RUN_SUITE(cvar_parse_with_range_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
