#include "../../thirdparty/minunit.h"

#include <stdbool.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

MU_TEST(test_window_create_negative_width_rejected) {
    window w = window_priv_create("t", -1, 600, 0);
    mu_check(w.handle == NULL);
}

MU_TEST(test_window_create_negative_height_rejected) {
    window w = window_priv_create("t", 800, -1, 0);
    mu_check(w.handle == NULL);
}

MU_TEST(test_window_create_zero_width_rejected) {
    window w = window_priv_create("t", 0, 600, 0);
    mu_check(w.handle == NULL);
}

MU_TEST(test_window_create_zero_height_rejected) {
    window w = window_priv_create("t", 800, 0, 0);
    mu_check(w.handle == NULL);
}

MU_TEST_SUITE(window_suite) {
    MU_RUN_TEST(test_window_create_negative_width_rejected);
    MU_RUN_TEST(test_window_create_negative_height_rejected);
    MU_RUN_TEST(test_window_create_zero_width_rejected);
    MU_RUN_TEST(test_window_create_zero_height_rejected);
}

int main(void) {
    MU_RUN_SUITE(window_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
