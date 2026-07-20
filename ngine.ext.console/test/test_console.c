#include "../../thirdparty/minunit.h"

#include <stdbool.h>
#include <string.h>

#define NGINE_UNIT_TEST
#undef ZOD_NGINE_IMPLEMENTATION
#define ZOD_NGINE_IMPLEMENTATION
#include "../index.h"

static void reset(void) { g_console = (console_state){.enabled = true}; }

MU_TEST(test_toggle_flips_visibility) {
    reset();
    mu_check(console_toggle() == true);
    mu_check(console_toggle() == false);
}

MU_TEST(test_toggle_noop_when_disabled) {
    g_console = (console_state){.enabled = false};
    mu_check(console_toggle() == false);
    mu_check(g_console.visible == false);
}

MU_TEST(test_apply_config_reads_enabled_cvar) {
    g_ctx = (engine_context){0};
    cvar_set_bool(&g_ctx.config.cvars, "console.enabled", true);
    reset();
    g_console.enabled = false;
    console_apply_config();
    mu_check(g_console.enabled == true);
    cvar_destroy(&g_ctx.config.cvars);
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
    mu_assert_int_eq(200, console_panel_height(10000, 10, 20, 0));
}

MU_TEST(test_panel_height_clamps_to_window_height) {
    mu_assert_int_eq(100, console_panel_height(100, 10, 20, 0));
}

MU_TEST(test_panel_height_includes_overhead) {
    // Regression: overhead (top_pad + input_gap) must be added on top of the
    // row budget, since console_platform_draw subtracts that same overhead
    // before computing how many rows fit — otherwise visible_lines rows
    // never actually fit in the returned height.
    mu_assert_int_eq(210, console_panel_height(10000, 10, 20, 10));
}

MU_TEST(test_visible_lines_plus_one_yields_exact_scrollback_rows) {
    // Regression: console_draw must budget visible_lines+1 total rows (the
    // extra +1 reserves the input row) so scrollback_rows comes out to
    // exactly visible_lines, not visible_lines-1 — this is the formula
    // console_draw and console_platform_draw's lines_fit must agree on.
    int visible_lines = 5;
    int row_height    = 20;
    int overhead      = 10;
    int height    = console_panel_height(10000, visible_lines + 1, row_height, overhead);
    int lines_fit = (height - overhead) / row_height;
    int scrollback_rows = lines_fit - 1;
    mu_assert_int_eq(visible_lines, scrollback_rows);
}

MU_TEST(test_draw_is_noop_when_hidden) {
    reset();
    mu_check(console_draw() == true);
}

MU_TEST(test_apply_config_defaults_visible_lines) {
    g_ctx = (engine_context){0};
    console_apply_config();
    mu_assert_int_eq(DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES, g_console.visible_lines);
}

MU_TEST(test_apply_config_reads_visible_lines_override) {
    g_ctx = (engine_context){0};
    cvar_set_int(&g_ctx.config.cvars, "console.visible_lines", 25);
    console_apply_config();
    mu_assert_int_eq(25, g_console.visible_lines);
    cvar_destroy(&g_ctx.config.cvars);
}

MU_TEST(test_visible_line_start_when_all_lines_fit) {
    mu_assert_int_eq(0, console_visible_line_start(5, 10));
}

MU_TEST(test_visible_line_start_when_buffer_exceeds_panel) {
    mu_assert_int_eq(90, console_visible_line_start(100, 10));
}

MU_TEST(test_input_append_stores_chars) {
    reset();
    console_input_append('h');
    console_input_append('i');
    mu_assert_string_eq("hi", g_console.input);
}

MU_TEST(test_input_append_bounded_when_full) {
    reset();
    for (int i = 0; i < CONSOLE_INPUT_MAX_LEN + 10; i++) console_input_append('a');
    mu_assert_int_eq(CONSOLE_INPUT_MAX_LEN - 1, (int)strlen(g_console.input));
}

MU_TEST(test_input_backspace_removes_last_char) {
    reset();
    console_input_append('h');
    console_input_append('i');
    console_input_backspace();
    mu_assert_string_eq("h", g_console.input);
}

MU_TEST(test_input_backspace_on_empty_is_noop) {
    reset();
    console_input_backspace();
    mu_assert_string_eq("", g_console.input);
}

MU_TEST(test_input_submit_echoes_and_clears) {
    reset();
    console_input_append('h');
    console_input_append('i');
    console_input_submit();

    mu_assert_int_eq(1, g_console.count);
    mu_assert_string_eq("hi", g_console.lines[0]);
    mu_assert_int_eq(0, g_console.input_len);
    mu_assert_string_eq("", g_console.input);
}

MU_TEST(test_input_submit_on_empty_is_noop) {
    reset();
    console_input_submit();
    mu_assert_int_eq(0, g_console.count);
}

MU_TEST(test_handle_event_noop_when_hidden) {
    reset();
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_TEXT, .text = "hi"});
    mu_assert_int_eq(0, g_console.input_len);
}

MU_TEST(test_handle_event_text_appends_when_visible) {
    reset();
    g_console.visible = true;
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_TEXT, .text = "hi"});
    mu_assert_string_eq("hi", g_console.input);
}

MU_TEST(test_handle_event_backspace_routes_correctly) {
    reset();
    g_console.visible = true;
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_TEXT, .text = "hi"});
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_BACKSPACE});
    mu_assert_string_eq("h", g_console.input);
}

MU_TEST(test_handle_event_submit_routes_correctly) {
    reset();
    g_console.visible = true;
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_TEXT, .text = "hi"});
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_SUBMIT});
    mu_assert_int_eq(1, g_console.count);
    mu_assert_string_eq("hi", g_console.lines[0]);
    mu_assert_int_eq(0, g_console.input_len);
}

MU_TEST(test_cursor_move_left_clamps_at_zero) {
    reset();
    console_input_move_left();
    mu_assert_int_eq(0, g_console.cursor_pos);
}

MU_TEST(test_cursor_move_right_clamps_at_input_len) {
    reset();
    console_input_append('h');
    console_input_append('i');
    console_input_move_right();
    console_input_move_right();
    mu_assert_int_eq(2, g_console.cursor_pos);
}

MU_TEST(test_cursor_moves_left_then_right) {
    reset();
    console_input_append('h');
    console_input_append('i');
    console_input_move_left();
    mu_assert_int_eq(1, g_console.cursor_pos);
    console_input_move_right();
    mu_assert_int_eq(2, g_console.cursor_pos);
}

MU_TEST(test_append_inserts_at_cursor_position) {
    reset();
    console_input_append('h');
    console_input_append('i');
    console_input_move_left();
    console_input_append('X');
    mu_assert_string_eq("hXi", g_console.input);
    mu_assert_int_eq(2, g_console.cursor_pos);
}

MU_TEST(test_backspace_removes_char_before_cursor) {
    reset();
    console_input_append('h');
    console_input_append('i');
    console_input_move_left();
    console_input_backspace();
    mu_assert_string_eq("i", g_console.input);
    mu_assert_int_eq(0, g_console.cursor_pos);
}

MU_TEST(test_handle_event_left_right_route_correctly) {
    reset();
    g_console.visible = true;
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_TEXT, .text = "hi"});
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_LEFT});
    mu_assert_int_eq(1, g_console.cursor_pos);
    console_handle_event((console_input_event){.kind = CONSOLE_INPUT_RIGHT});
    mu_assert_int_eq(2, g_console.cursor_pos);
}

MU_TEST_SUITE(console_suite) {
    MU_RUN_TEST(test_toggle_flips_visibility);
    MU_RUN_TEST(test_toggle_noop_when_disabled);
    MU_RUN_TEST(test_apply_config_reads_enabled_cvar);
    MU_RUN_TEST(test_write_stores_plain_string);
    MU_RUN_TEST(test_write_formats_args);
    MU_RUN_TEST(test_write_overflow_drops_oldest_line);
    MU_RUN_TEST(test_write_truncates_overlong_line);
    MU_RUN_TEST(test_panel_height_uses_visible_lines_when_window_tall_enough);
    MU_RUN_TEST(test_panel_height_clamps_to_window_height);
    MU_RUN_TEST(test_panel_height_includes_overhead);
    MU_RUN_TEST(test_visible_lines_plus_one_yields_exact_scrollback_rows);
    MU_RUN_TEST(test_draw_is_noop_when_hidden);
    MU_RUN_TEST(test_apply_config_defaults_visible_lines);
    MU_RUN_TEST(test_apply_config_reads_visible_lines_override);
    MU_RUN_TEST(test_visible_line_start_when_all_lines_fit);
    MU_RUN_TEST(test_visible_line_start_when_buffer_exceeds_panel);
    MU_RUN_TEST(test_input_append_stores_chars);
    MU_RUN_TEST(test_input_append_bounded_when_full);
    MU_RUN_TEST(test_input_backspace_removes_last_char);
    MU_RUN_TEST(test_input_backspace_on_empty_is_noop);
    MU_RUN_TEST(test_input_submit_echoes_and_clears);
    MU_RUN_TEST(test_input_submit_on_empty_is_noop);
    MU_RUN_TEST(test_handle_event_noop_when_hidden);
    MU_RUN_TEST(test_handle_event_text_appends_when_visible);
    MU_RUN_TEST(test_handle_event_backspace_routes_correctly);
    MU_RUN_TEST(test_handle_event_submit_routes_correctly);
    MU_RUN_TEST(test_cursor_move_left_clamps_at_zero);
    MU_RUN_TEST(test_cursor_move_right_clamps_at_input_len);
    MU_RUN_TEST(test_cursor_moves_left_then_right);
    MU_RUN_TEST(test_append_inserts_at_cursor_position);
    MU_RUN_TEST(test_backspace_removes_char_before_cursor);
    MU_RUN_TEST(test_handle_event_left_right_route_correctly);
}

int main(void) {
    MU_RUN_SUITE(console_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
