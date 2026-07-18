#include "../../lib/minunit.h"

#include <stdbool.h>
#include <string.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static void reset(void) { g_console = (console_state){0}; }

MU_TEST(test_toggle_flips_visibility) {
    reset();
    mu_check(console_toggle() == true);
    mu_check(console_toggle() == false);
}

MU_TEST(test_write_stores_plain_string) {
    reset();
    console_write("hello");
    mu_assert_int_eq(1, g_console.count);
    mu_assert_string_eq("hello", g_console.lines[0]);
}

MU_TEST(test_write_formats_args) {
    reset();
    console_write("fps: %d", 60);
    mu_assert_string_eq("fps: 60", g_console.lines[0]);
}

MU_TEST(test_write_overflow_drops_oldest_line) {
    reset();
    for (int i = 0; i < CONSOLE_MAX_LINES; i++) console_write("line %d", i);
    console_write("overflow");

    mu_assert_int_eq(CONSOLE_MAX_LINES, g_console.count);
    mu_assert_string_eq("line 1", g_console.lines[0]);
    mu_assert_string_eq("overflow", g_console.lines[CONSOLE_MAX_LINES - 1]);
}

MU_TEST(test_write_truncates_overlong_line) {
    reset();
    char long_arg[CONSOLE_MAX_LINE_LEN * 2];
    memset(long_arg, 'a', sizeof(long_arg) - 1);
    long_arg[sizeof(long_arg) - 1] = '\0';

    console_write("%s", long_arg);

    mu_assert_int_eq(CONSOLE_MAX_LINE_LEN - 1, (int)strlen(g_console.lines[0]));
}

MU_TEST(test_panel_height_uses_visible_lines_when_window_tall_enough) {
    mu_assert_int_eq(10 * CONSOLE_LINE_HEIGHT, console_panel_height(10000, 10));
}

MU_TEST(test_panel_height_clamps_to_window_height) {
    mu_assert_int_eq(100, console_panel_height(100, 10));
}

MU_TEST(test_draw_is_noop_when_hidden) {
    reset();
    mu_check(console_draw() == true);
}

MU_TEST(test_zod_console_toggle_forwards) {
    reset();
    mu_check(zod_console_toggle() == true);
    mu_check(g_console.visible == true);
}

MU_TEST(test_resolve_visible_lines_defaults) {
    g_ctx = (engine_context){0};
    mu_assert_int_eq(DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES, console_resolve_visible_lines());
}

MU_TEST(test_resolve_visible_lines_reads_cvar_override) {
    g_ctx = (engine_context){0};
    cvar_set_int(&g_ctx.config.cvars, "console.visible_lines", 25);
    mu_assert_int_eq(25, console_resolve_visible_lines());
}

MU_TEST(test_visible_line_start_when_all_lines_fit) {
    mu_assert_int_eq(0, console_visible_line_start(5, 10));
}

MU_TEST(test_visible_line_start_when_buffer_exceeds_panel) {
    mu_assert_int_eq(90, console_visible_line_start(100, 10));
}

MU_TEST(test_zod_console_write_forwards) {
    reset();
    zod_console_write("fps: %d", 60);
    mu_assert_string_eq("fps: 60", g_console.lines[0]);
}

MU_TEST_SUITE(console_suite) {
    MU_RUN_TEST(test_toggle_flips_visibility);
    MU_RUN_TEST(test_write_stores_plain_string);
    MU_RUN_TEST(test_write_formats_args);
    MU_RUN_TEST(test_write_overflow_drops_oldest_line);
    MU_RUN_TEST(test_write_truncates_overlong_line);
    MU_RUN_TEST(test_panel_height_uses_visible_lines_when_window_tall_enough);
    MU_RUN_TEST(test_panel_height_clamps_to_window_height);
    MU_RUN_TEST(test_draw_is_noop_when_hidden);
    MU_RUN_TEST(test_zod_console_toggle_forwards);
    MU_RUN_TEST(test_zod_console_write_forwards);
    MU_RUN_TEST(test_resolve_visible_lines_defaults);
    MU_RUN_TEST(test_resolve_visible_lines_reads_cvar_override);
    MU_RUN_TEST(test_visible_line_start_when_all_lines_fit);
    MU_RUN_TEST(test_visible_line_start_when_buffer_exceeds_panel);
}

int main(void) {
    MU_RUN_SUITE(console_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
