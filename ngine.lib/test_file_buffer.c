#include "../thirdparty/minunit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IO_IMPLEMENTATION
#include "file_buffer.h"

MU_TEST(test_read_file_success) {
    const char *filename = "test_data.txt";
    const char *content =
         "#version 330 core\n"
         "out vec4 FragColor;\n"
         "void main() {\n"
         "    FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n"
         "}\n";

    FILE *f = fopen(filename, "w");
    fprintf(f, "%s", content);
    fclose(f);

    const file_buffer *fb = read_file_as_string(filename);

    mu_check(fb != NULL);
    mu_check(fb->size == strlen(content));
    mu_assert_string_eq((const char *)fb->data, content);

    free_file_buffer(fb);
    remove(filename);
}

MU_TEST(test_read_file_nonexistent) {
    const file_buffer *fb = read_file_as_string("does_not_exist.txt");
    mu_check(fb == NULL);
}

MU_TEST(test_read_file_empty) {
    const char *filename = "empty.txt";

    FILE *f = fopen(filename, "w");
    fclose(f);

    const file_buffer *fb = read_file_as_string(filename);

    mu_check(fb != NULL);
    mu_check(fb->size == 0);
    mu_assert_string_eq((const char *)fb->data, "");

    free_file_buffer(fb);
    remove(filename);
}

MU_TEST(test_read_file_binary_success) {
    const char *filename = "bin.dat";
    uint8_t     data[]   = {1, 2, 3, 4, 5};

    FILE *f = fopen(filename, "wb");
    fwrite(data, 1, sizeof(data), f);
    fclose(f);

    const file_buffer *fb = read_file_as_binary(filename);

    mu_check(fb != NULL);
    mu_check(fb->size == sizeof(data));
    mu_check(memcmp(fb->data, data, sizeof(data)) == 0);

    free_file_buffer(fb);
    remove(filename);
}

MU_TEST(test_read_file_binary_nonexistent) {
    const file_buffer *fb = read_file_as_binary("nope.bin");
    mu_check(fb == NULL);
}

MU_TEST_SUITE(file_buffer_tests) {
    MU_RUN_TEST(test_read_file_success);
    MU_RUN_TEST(test_read_file_nonexistent);
    MU_RUN_TEST(test_read_file_empty);
    MU_RUN_TEST(test_read_file_binary_success);
    MU_RUN_TEST(test_read_file_binary_nonexistent);
}

int main(void) {
    MU_RUN_SUITE(file_buffer_tests);
    MU_REPORT();
    return MU_EXIT_CODE;
}
