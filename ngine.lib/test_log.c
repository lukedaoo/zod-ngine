#include "../thirdparty/minunit.h"

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
    log_set_fp(fp, LOG_WARN);      // file only takes WARN and above

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

    char   buf[256];
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

MU_TEST(test_string_to_log_level) {
    mu_check(log_level_from_string("trace") == LOG_TRACE);
    mu_check(log_level_from_string("debug") == LOG_DEBUG);
    mu_check(log_level_from_string("info") == LOG_INFO);
    mu_check(log_level_from_string("warn") == LOG_WARN);
    mu_check(log_level_from_string("error") == LOG_ERROR);
    mu_check(log_level_from_string("fatal") == LOG_FATAL);
    mu_check(log_level_from_string("unknown") == LOG_FATAL);

    mu_check(log_level_from_string(NULL) == LOG_FATAL);
    mu_check(log_level_from_string("") == LOG_FATAL);

    mu_check(log_level_from_string("Trace") == LOG_TRACE);
    mu_check(log_level_from_string("Debug") == LOG_DEBUG);
    mu_check(log_level_from_string("Info") == LOG_INFO);
    mu_check(log_level_from_string("Warn") == LOG_WARN);
    mu_check(log_level_from_string("Error") == LOG_ERROR);
    mu_check(log_level_from_string("Fatal") == LOG_FATAL);

    mu_check(log_level_from_string("TRACE") == LOG_TRACE);
    mu_check(log_level_from_string("DEBUG") == LOG_DEBUG);
    mu_check(log_level_from_string("INFO") == LOG_INFO);
    mu_check(log_level_from_string("WARN") == LOG_WARN);
    mu_check(log_level_from_string("ERROR") == LOG_ERROR);
    mu_check(log_level_from_string("FATAL") == LOG_FATAL);
}

static int  hook_last_level = -1;
static char hook_last_message[256];
static int  hook_call_count = 0;

static void test_hook_capture(int level, const char *message) {
    hook_last_level = level;
    strncpy(hook_last_message, message, sizeof(hook_last_message) - 1);
    hook_last_message[sizeof(hook_last_message) - 1] = '\0';
    hook_call_count++;
}

MU_TEST(test_log_hook_respects_log_level) {
    hook_call_count = 0;
    log_register_hook(test_hook_capture);
    log_set_fp(NULL, 0);  // mute file sink, isolate the hook from it

    log_set_level(LOG_ERROR);  // hooks now gate on the same threshold as stderr

    log_warn("below threshold");
    mu_check(hook_call_count == 0);

    log_error("hook saw %d", 7);
    mu_check(hook_call_count == 1);
    mu_check(hook_last_level == LOG_ERROR);
    mu_check(strstr(hook_last_message, "hook saw 7") != NULL);

    log_set_level(LOG_TRACE);  // restore default for subsequent tests
}

MU_TEST(test_log_hook_table_full_is_dropped_safely) {
    // one hook already registered by the previous test; fill remaining slots
    for (int i = 1; i < LOG_MAX_HOOKS; i++) {
        log_register_hook(test_hook_capture);
    }
    log_register_hook(test_hook_capture);  // table full -> dropped, no crash

    hook_call_count = 0;
    log_warn("fanout");
    mu_check(hook_call_count == LOG_MAX_HOOKS);
}

static void test_hook_noop(int level, const char *message) {
    (void)level;
    (void)message;
}

MU_TEST(test_log_unregister_hook_stops_firing) {
    // previous tests left LOG_MAX_HOOKS copies of test_hook_capture registered;
    // clear them all out first so this test starts from a known state
    for (int i = 0; i < LOG_MAX_HOOKS; i++) {
        log_unregister_hook(test_hook_capture);
    }

    log_register_hook(test_hook_capture);
    log_register_hook(test_hook_noop);

    hook_call_count = 0;
    log_warn("before unregister");
    mu_check(hook_call_count == 1);

    log_unregister_hook(test_hook_capture);

    hook_call_count = 0;
    log_warn("after unregister");
    mu_check(hook_call_count == 0);  // capture hook gone; noop hook still registered

    log_unregister_hook(test_hook_noop);  // clean slate for any future tests
}

MU_TEST_SUITE(log_suite) {
    MU_RUN_TEST(test_log_writes_to_file);
    MU_RUN_TEST(test_log_file_level_threshold);
    MU_RUN_TEST(test_log_disable_via_null);
    MU_RUN_TEST(test_string_to_log_level);
    MU_RUN_TEST(test_log_hook_respects_log_level);
    MU_RUN_TEST(test_log_hook_table_full_is_dropped_safely);
    MU_RUN_TEST(test_log_unregister_hook_stops_firing);
}

int main(void) {
    MU_RUN_SUITE(log_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
