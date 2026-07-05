#include "../../lib/minunit.h"

#include <stdbool.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static void reset(void) { g_ctx.clock = (engine_clock){0}; }

MU_TEST(test_clock_init_positive_fps_sets_frame_rate) {
    reset();
    clock_init(60);
    mu_assert_int_eq(60, (int)g_ctx.clock.frame_rate);
    mu_check(g_ctx.clock.frame_delay > 0.0f);
}

MU_TEST(test_clock_init_zero_fps_uncapped) {
    reset();
    clock_init(0);
    mu_assert_int_eq(0, (int)g_ctx.clock.frame_rate);
    mu_check(g_ctx.clock.frame_delay == 0.0f);
}

MU_TEST(test_clock_init_negative_fps_rejected) {
    reset();
    clock_init((uint32_t)-22);
    mu_assert_int_eq(0, (int)g_ctx.clock.frame_rate);
    mu_check(g_ctx.clock.frame_delay == 0.0f);
}

MU_TEST(test_clock_change_target_fps_positive_applies) {
    reset();
    clock_init(30);
    clock_change_target_fps(120);
    mu_assert_int_eq(120, (int)g_ctx.clock.frame_rate);
}

MU_TEST(test_clock_change_target_fps_negative_ignored) {
    reset();
    clock_init(30);
    clock_change_target_fps((uint32_t)-1);
    mu_assert_int_eq(30, (int)g_ctx.clock.frame_rate);
}

MU_TEST_SUITE(clock_suite) {
    MU_RUN_TEST(test_clock_init_positive_fps_sets_frame_rate);
    MU_RUN_TEST(test_clock_init_zero_fps_uncapped);
    MU_RUN_TEST(test_clock_init_negative_fps_rejected);
    MU_RUN_TEST(test_clock_change_target_fps_positive_applies);
    MU_RUN_TEST(test_clock_change_target_fps_negative_ignored);
}

int main(void) {
    MU_RUN_SUITE(clock_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
