#include "../lib/minunit.h"

#include <string.h>

#define LOG_IMPLEMENTATION
#include "log.h"

// Read the whole file sink back into `out` (NUL-terminated). Returns length.
static size_t read_back(FILE *fp, char *out, size_t cap) {
    fflush(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) return 0;
    size_t n = fread(out, 1, cap - 1, fp);
    out[n]   = '\0';
    return n;
}

MU_TEST(test_log_writes_to_file) {
    FILE *fp = tmpfile();
    mu_check(fp != NULL);

    log_set_level(LOG_FATAL + 1);  // mute console; file sink has its own level
    log_set_fp(fp, LOG_TRACE);

    log_info("x=%d", 1);

    char buf[256];
    read_back(fp, buf, sizeof(buf));
    mu_check(strstr(buf, "INFO") != NULL);
    mu_check(strstr(buf, "x=1") != NULL);

    log_set_fp(NULL, 0);
    fclose(fp);
}

MU_TEST(test_log_file_level_threshold) {
    FILE *fp = tmpfile();
    mu_check(fp != NULL);

    log_set_level(LOG_FATAL + 1);  // mute console; file sink has its own level
    log_set_fp(fp, LOG_WARN);  // file only takes WARN and above

    log_info("below threshold");
    char buf[256];
    read_back(fp, buf, sizeof(buf));
    mu_check(buf[0] == '\0');  // nothing written

    log_error("above threshold");
    read_back(fp, buf, sizeof(buf));
    mu_check(strstr(buf, "ERROR") != NULL);
    mu_check(strstr(buf, "above threshold") != NULL);
    mu_check(strstr(buf, "below threshold") == NULL);

    log_set_fp(NULL, 0);
    fclose(fp);
}

MU_TEST(test_log_disable_via_null) {
    FILE *fp = tmpfile();
    mu_check(fp != NULL);

    log_set_level(LOG_FATAL + 1);  // mute console; file sink has its own level
    log_set_fp(fp, LOG_TRACE);
    log_info("first");

    char buf[256];
    size_t len_before = read_back(fp, buf, sizeof(buf));
    mu_check(len_before > 0);

    // disable, then log again -> file must not grow
    log_set_fp(NULL, 0);
    log_info("second");

    size_t len_after = read_back(fp, buf, sizeof(buf));
    mu_check(len_after == len_before);
    mu_check(strstr(buf, "second") == NULL);

    fclose(fp);
}

MU_TEST_SUITE(log_suite) {
    MU_RUN_TEST(test_log_writes_to_file);
    MU_RUN_TEST(test_log_file_level_threshold);
    MU_RUN_TEST(test_log_disable_via_null);
}

int main(void) {
    MU_RUN_SUITE(log_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
