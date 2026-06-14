#include "../lib/minunit.h"

#include <string.h>

#define INI_IMPLEMENTATION
#include "ini.h"

#define MAX_CAPTURED 16

typedef struct {
    char section[32];
    char key[32];
    char value[32];
} CapturedEntry;

typedef struct {
    CapturedEntry entries[MAX_CAPTURED];
    int           count;
    int           limit;
} Captured;

static bool capture_handler(const char *section,
                            const char *key,
                            const char *value,
                            void       *user) {
    Captured *cap = user;
    if (cap->count >= MAX_CAPTURED) return false;

    CapturedEntry *e = &cap->entries[cap->count];
    strncpy(e->section, section, sizeof(e->section) - 1);
    e->section[sizeof(e->section) - 1] = '\0';
    strncpy(e->key, key, sizeof(e->key) - 1);
    e->key[sizeof(e->key) - 1] = '\0';
    strncpy(e->value, value, sizeof(e->value) - 1);
    e->value[sizeof(e->value) - 1] = '\0';
    cap->count++;

    return !(cap->limit > 0 && cap->count >= cap->limit);
}

MU_TEST(test_parse_string_sections_and_duplicate_keys) {
    const char *ini =
         "[group1]\n"
         "value   =   1\n"
         "value   =   2\n"
         "\n"
         "[group2]\n"
         "value   =   3\n";

    Captured cap = {0};
    bool     rc  = ini_parse_string(ini, capture_handler, &cap);

    mu_check(rc);
    mu_assert_int_eq(3, cap.count);

    mu_assert_string_eq("group1", cap.entries[0].section);
    mu_assert_string_eq("value", cap.entries[0].key);
    mu_assert_string_eq("1", cap.entries[0].value);

    mu_assert_string_eq("group1", cap.entries[1].section);
    mu_assert_string_eq("2", cap.entries[1].value);

    mu_assert_string_eq("group2", cap.entries[2].section);
    mu_assert_string_eq("3", cap.entries[2].value);
}

MU_TEST(test_parse_string_comments_and_leading_whitespace) {
    const char *ini =
         "; leading comment\n"
         "# another comment\n"
         "\n"
         "[s]\n"
         "  key = val\n";

    Captured cap = {0};
    bool     rc  = ini_parse_string(ini, capture_handler, &cap);

    mu_check(rc);
    mu_assert_int_eq(1, cap.count);
    mu_assert_string_eq("s", cap.entries[0].section);
    mu_assert_string_eq("key", cap.entries[0].key);
    mu_assert_string_eq("val", cap.entries[0].value);
}

MU_TEST(test_parse_string_no_trailing_newline) {
    const char *ini = "[s]\nkey=val";

    Captured cap = {0};
    bool     rc  = ini_parse_string(ini, capture_handler, &cap);

    mu_check(rc);
    mu_assert_int_eq(1, cap.count);
    mu_assert_string_eq("val", cap.entries[0].value);
}

MU_TEST(test_parse_string_handler_failure_stops_parsing) {
    const char *ini =
         "[s]\n"
         "a = 1\n"
         "b = 2\n";

    Captured cap = {0};
    cap.limit    = 1;
    bool rc      = ini_parse_string(ini, capture_handler, &cap);

    mu_check(!rc);
    mu_assert_int_eq(1, cap.count);
}

MU_TEST(test_parse_string_inline_comment_stripped) {
    const char *ini =
         "[s]\n"
         "a=123 #test\n"
         "b = 456 ; trailing\n"
         "c = 789\n";

    Captured cap = {0};
    bool     rc  = ini_parse_string(ini, capture_handler, &cap);

    mu_check(rc);
    mu_assert_int_eq(3, cap.count);

    mu_assert_string_eq("123", cap.entries[0].value);
    mu_assert_string_eq("456", cap.entries[1].value);

    mu_assert_string_eq("c", cap.entries[2].key);
    mu_assert_string_eq("789", cap.entries[2].value);
}

MU_TEST(test_parse_string_value_starting_with_hash_not_comment) {
    const char *ini =
         "[s]\n"
         "color=#ff0000\n";

    Captured cap = {0};
    bool     rc  = ini_parse_string(ini, capture_handler, &cap);

    mu_check(rc);
    mu_assert_int_eq(1, cap.count);

    mu_assert_string_eq("#ff0000", cap.entries[0].value);
}

MU_TEST(test_parse_string_empty_input) {
    Captured cap = {0};
    bool     rc  = ini_parse_string("", capture_handler, &cap);

    mu_check(rc);
    mu_assert_int_eq(0, cap.count);
}

MU_TEST_SUITE(ini_suite) {
    MU_RUN_TEST(test_parse_string_sections_and_duplicate_keys);
    MU_RUN_TEST(test_parse_string_comments_and_leading_whitespace);
    MU_RUN_TEST(test_parse_string_no_trailing_newline);
    MU_RUN_TEST(test_parse_string_handler_failure_stops_parsing);
    MU_RUN_TEST(test_parse_string_inline_comment_stripped);
    MU_RUN_TEST(test_parse_string_value_starting_with_hash_not_comment);
    MU_RUN_TEST(test_parse_string_empty_input);
}

int main(void) {
    MU_RUN_SUITE(ini_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
