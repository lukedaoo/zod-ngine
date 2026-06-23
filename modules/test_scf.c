#include "../lib/minunit.h"

#include <stdbool.h>
#include <string.h>

#define SCF_IMPLEMENTATION
#include "scf.h"

typedef struct {
    char section[128];
    char key[128];
    char value[128];
    int  calls;
} capture;

static bool capture_handler(const char *section,
                             const char *key,
                             const char *value,
                             void       *user) {
    capture *c = user;
    strncpy(c->section, section, sizeof(c->section) - 1);
    strncpy(c->key, key, sizeof(c->key) - 1);
    strncpy(c->value, value, sizeof(c->value) - 1);
    c->calls++;
    return true;
}

static bool counting_handler(const char *section,
                              const char *key,
                              const char *value,
                              void       *user) {
    (void)section;
    (void)key;
    (void)value;
    int *count = user;
    (*count)++;
    return true;
}

static bool stop_after_first_handler(const char *section,
                                      const char *key,
                                      const char *value,
                                      void       *user) {
    (void)section;
    (void)key;
    (void)value;
    int *count = user;
    (*count)++;
    return *count < 1;
}

MU_TEST(test_section_and_key_value) {
    capture c = {0};
    mu_check(scf_parse_string(":/display\nvsync true\n", capture_handler, &c));
    mu_assert_string_eq("display", c.section);
    mu_assert_string_eq("vsync", c.key);
    mu_assert_string_eq("true", c.value);
}

MU_TEST(test_full_line_comment_semicolon) {
    int count = 0;
    mu_check(scf_parse_string("; just a comment\n", counting_handler, &count));
    mu_check(count == 0);
}

MU_TEST(test_full_line_comment_hash_is_not_a_comment) {
    capture c = {0};
    mu_check(scf_parse_string(":/x\n#key value\n", capture_handler, &c));
    mu_assert_string_eq("#key", c.key);
    mu_assert_string_eq("value", c.value);
}

MU_TEST(test_inline_comment_semicolon) {
    capture c = {0};
    mu_check(scf_parse_string(":/display\nvsync true ; enable vsync\n", capture_handler, &c));
    mu_assert_string_eq("true", c.value);
}

MU_TEST(test_value_starting_with_hash_stays_literal) {
    capture c = {0};
    mu_check(scf_parse_string(":/display\nurl #ff0000\n", capture_handler, &c));
    mu_assert_string_eq("#ff0000", c.value);
}

MU_TEST(test_leading_and_trailing_whitespace_stripped) {
    capture c = {0};
    mu_check(scf_parse_string(":/display\n  vsync   true  \n", capture_handler, &c));
    mu_assert_string_eq("vsync", c.key);
    mu_assert_string_eq("true", c.value);
}

MU_TEST(test_empty_and_comment_only_lines_skipped) {
    int count = 0;
    mu_check(scf_parse_string("\n   \n; comment\n", counting_handler, &count));
    mu_check(count == 0);
}

MU_TEST(test_malformed_line_missing_value_skipped) {
    int count = 0;
    mu_check(scf_parse_string(":/x\nkeyonly\n", counting_handler, &count));
    mu_check(count == 0);
}

MU_TEST(test_missing_handler_returns_false) {
    mu_check(!scf_parse_string(":/x\nkey value\n", NULL, NULL));
}

MU_TEST(test_null_string_returns_false) {
    int count = 0;
    mu_check(!scf_parse_string(NULL, counting_handler, &count));
    mu_check(count == 0);
}

MU_TEST(test_handler_false_stops_parsing) {
    int count = 0;
    mu_check(!scf_parse_string(":/x\na 1\nb 2\nc 3\n", stop_after_first_handler, &count));
    mu_check(count == 1);
}

MU_TEST(test_section_persists_across_keys) {
    capture c = {0};
    mu_check(scf_parse_string(":/audio\nmaster_volumn 1.0F\n", capture_handler, &c));
    mu_assert_string_eq("audio", c.section);

    mu_check(scf_parse_string(":/audio\n:/display\nvsync true\n", capture_handler, &c));
    mu_assert_string_eq("display", c.section);
}

MU_TEST_SUITE(scf_string_suite) {
    MU_RUN_TEST(test_section_and_key_value);
    MU_RUN_TEST(test_full_line_comment_semicolon);
    MU_RUN_TEST(test_full_line_comment_hash_is_not_a_comment);
    MU_RUN_TEST(test_inline_comment_semicolon);
    MU_RUN_TEST(test_value_starting_with_hash_stays_literal);
    MU_RUN_TEST(test_leading_and_trailing_whitespace_stripped);
    MU_RUN_TEST(test_empty_and_comment_only_lines_skipped);
    MU_RUN_TEST(test_malformed_line_missing_value_skipped);
    MU_RUN_TEST(test_missing_handler_returns_false);
    MU_RUN_TEST(test_null_string_returns_false);
    MU_RUN_TEST(test_handler_false_stops_parsing);
    MU_RUN_TEST(test_section_persists_across_keys);
}

#ifdef _WIN32
#define TEST_SCF_PATH "tmp/scf_test.scf"
#else
#define TEST_SCF_PATH "/tmp/scf_test.scf"
#endif

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    fputs(content, f);
    fclose(f);
}

MU_TEST(test_scf_parse_from_file) {
    write_file(TEST_SCF_PATH, ":/display\nvsync true ; enable vsync\n");
    capture c = {0};
    mu_check(scf_parse(TEST_SCF_PATH, capture_handler, &c));
    mu_assert_string_eq("display", c.section);
    mu_assert_string_eq("vsync", c.key);
    mu_assert_string_eq("true", c.value);
    remove(TEST_SCF_PATH);
}

MU_TEST(test_scf_parse_missing_file_returns_false) {
    mu_check(!scf_parse("/nonexistent/path/scf_test.scf", capture_handler, NULL));
}

MU_TEST_SUITE(scf_file_suite) {
    MU_RUN_TEST(test_scf_parse_from_file);
    MU_RUN_TEST(test_scf_parse_missing_file_returns_false);
}

int main(void) {
    MU_RUN_SUITE(scf_string_suite);
    MU_RUN_SUITE(scf_file_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
