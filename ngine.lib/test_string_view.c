#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define STRING_VIEW_IMPLEMENTATION
#include "string_view.h"

static void capture_print(string_view view, char *buffer, size_t buffer_size) {
    FILE *tmp = tmpfile();
    mu_check(tmp != NULL);

    fflush(stdout);
    int saved_fd = dup(STDOUT_FILENO);
    dup2(fileno(tmp), STDOUT_FILENO);

    string_view_print(view);

    fflush(stdout);
    dup2(saved_fd, STDOUT_FILENO);
    close(saved_fd);

    fseek(tmp, 0, SEEK_SET);
    size_t n  = fread(buffer, 1, buffer_size - 1, tmp);
    buffer[n] = '\0';
    fclose(tmp);
}

MU_TEST(test_init_stores_data_and_size) {
    const char *text = "hello";
    string_view sv   = string_view_init(text, 5);

    mu_check(sv.data == text);
    mu_assert_int_eq(5, (int)sv.size);
}

MU_TEST(test_init_zero_size) {
    string_view sv = string_view_init("", 0);

    mu_assert_int_eq(0, (int)sv.size);
}

MU_TEST(test_init_partial_view) {
    const char *text = "hello world";
    string_view sv   = string_view_init(text, 5);

    mu_assert_int_eq(5, (int)sv.size);
    mu_check(memcmp(sv.data, "hello", 5) == 0);
}

MU_TEST(test_equals_identical_content) {
    string_view a = string_view_init("hello", 5);
    string_view b = string_view_init("hello", 5);

    mu_check(string_view_equals(a, b));
}

MU_TEST(test_equals_same_view_self) {
    string_view a = string_view_init("hello", 5);

    mu_check(string_view_equals(a, a));
}

MU_TEST(test_equals_different_content_same_size) {
    string_view a = string_view_init("hello", 5);
    string_view b = string_view_init("world", 5);

    mu_check(!string_view_equals(a, b));
}

MU_TEST(test_equals_different_size) {
    string_view a = string_view_init("hello", 5);
    string_view b = string_view_init("hello!", 6);

    mu_check(!string_view_equals(a, b));
}

MU_TEST(test_equals_prefix_with_matching_size) {
    string_view a = string_view_init("hello", 5);
    string_view b = string_view_init("hello world", 5);

    mu_check(string_view_equals(a, b));
}

MU_TEST(test_equals_case_sensitive) {
    string_view a = string_view_init("Hello", 5);
    string_view b = string_view_init("hello", 5);

    mu_check(!string_view_equals(a, b));
}

MU_TEST(test_equals_empty_views) {
    string_view a = string_view_init("", 0);
    string_view b = string_view_init("anything", 0);

    mu_check(string_view_equals(a, b));
}

MU_TEST(test_print_full_view) {
    string_view sv = string_view_init("hello", 5);

    char buffer[32];
    capture_print(sv, buffer, sizeof(buffer));

    mu_assert_string_eq("hello", buffer);
}

MU_TEST(test_print_partial_view) {
    string_view sv = string_view_init("hello world", 5);

    char buffer[32];
    capture_print(sv, buffer, sizeof(buffer));

    mu_assert_string_eq("hello", buffer);
}

MU_TEST(test_print_empty_view) {
    string_view sv = string_view_init("", 0);

    char buffer[32];
    capture_print(sv, buffer, sizeof(buffer));

    mu_assert_string_eq("", buffer);
}

MU_TEST(test_macro_initialization) {
    string_view sv = SV("hello");
    mu_assert_int_eq(5, (int)sv.size);

    char        buffer[] = "hello, world";
    char       *p        = buffer;
    string_view sv2      = SV_STR(p);
    mu_assert_int_eq(12, (int)sv2.size);
}

MU_TEST_SUITE(string_view_suite) {
    MU_RUN_TEST(test_init_stores_data_and_size);
    MU_RUN_TEST(test_init_zero_size);
    MU_RUN_TEST(test_init_partial_view);

    MU_RUN_TEST(test_equals_identical_content);
    MU_RUN_TEST(test_equals_same_view_self);
    MU_RUN_TEST(test_equals_different_content_same_size);
    MU_RUN_TEST(test_equals_different_size);
    MU_RUN_TEST(test_equals_prefix_with_matching_size);
    MU_RUN_TEST(test_equals_case_sensitive);
    MU_RUN_TEST(test_equals_empty_views);

    MU_RUN_TEST(test_print_full_view);
    MU_RUN_TEST(test_print_partial_view);
    MU_RUN_TEST(test_print_empty_view);
    MU_RUN_TEST(test_macro_initialization);
}

int main(void) {
    MU_RUN_SUITE(string_view_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
