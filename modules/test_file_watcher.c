#include "../lib/minunit.h"

#include <stdio.h>
#include <unistd.h>

#define LOG_IMPLEMENTATION
#define FILE_WATCHER_IMPLEMENTATION
#include "file_watcher.h"

#ifdef _WIN32
#define TEST_FILE "tmp/file_watcher_test.txt"
#else
#define TEST_FILE "/tmp/file_watcher_test.txt"
#endif

static void touch(const char *path) {
    FILE *f = fopen(path, "w");
    fclose(f);
}

MU_TEST(test_watch_existing_file_first_check_none) {
    remove(TEST_FILE);
    touch(TEST_FILE);

    file_watcher *w = file_watcher_watch(TEST_FILE);
    mu_check(w != NULL);
    mu_check(file_watcher_check(w) == FILE_NONE);

    file_watcher_close(w);
    remove(TEST_FILE);
}

MU_TEST(test_modify_detected) {
    remove(TEST_FILE);
    touch(TEST_FILE);

    file_watcher *w = file_watcher_watch(TEST_FILE);
    mu_check(w != NULL);
    mu_check(file_watcher_check(w) == FILE_NONE);

    sleep(1);
    touch(TEST_FILE);
    mu_check(file_watcher_check(w) == FILE_CHANGED);

    file_watcher_close(w);
    remove(TEST_FILE);
}

MU_TEST(test_delete_and_recreate_detected) {
    remove(TEST_FILE);
    touch(TEST_FILE);

    file_watcher *w = file_watcher_watch(TEST_FILE);
    mu_check(w != NULL);
    mu_check(file_watcher_check(w) == FILE_NONE);

    remove(TEST_FILE);
    mu_check(file_watcher_check(w) == FILE_DELETED);

    touch(TEST_FILE);
    mu_check(file_watcher_check(w) == FILE_NEW);

    file_watcher_close(w);
    remove(TEST_FILE);
}

MU_TEST(test_watch_nonexistent_then_create) {
    remove(TEST_FILE);

    file_watcher *w = file_watcher_watch(TEST_FILE);
    mu_check(w != NULL);
    mu_check(file_watcher_check(w) == FILE_NONE);

    touch(TEST_FILE);
    mu_check(file_watcher_check(w) == FILE_NEW);

    file_watcher_close(w);
    remove(TEST_FILE);
}

MU_TEST(test_watch_directory_rejected) {
#ifdef _WIN32
    file_watcher *w = file_watcher_watch("tmp");
#else
    file_watcher *w = file_watcher_watch("/tmp");
#endif
    mu_check(w == NULL);
}

MU_TEST(test_watch_path_too_long_rejected) {
    char path[300];
    for (int i = 0; i < 299; i++) path[i] = 'a';
    path[299] = '\0';

    file_watcher *w = file_watcher_watch(path);
    mu_check(w == NULL);
}

MU_TEST_SUITE(file_watcher_suite) {
    MU_RUN_TEST(test_watch_existing_file_first_check_none);
    MU_RUN_TEST(test_modify_detected);
    MU_RUN_TEST(test_delete_and_recreate_detected);
    MU_RUN_TEST(test_watch_nonexistent_then_create);
    MU_RUN_TEST(test_watch_directory_rejected);
    MU_RUN_TEST(test_watch_path_too_long_rejected);
}

int main(void) {
    MU_RUN_SUITE(file_watcher_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
