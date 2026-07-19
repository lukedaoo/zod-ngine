#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "../../console.h"
#include "ngine.core/internal/engine_context/engine_context_internal.h"
#include "ngine.core/zod_ngine.h"
#include "console_internal.h"

#if ZOD_CONSOLE_ENABLED

static const cvar_constraint g_console_constraints[] = {
     {.name = "console.enabled", .expected = CVAR_BOOL},
     {.name = "console.text_pad_x", .expected = CVAR_FLOAT},
     {.name = "console.top_pad", .expected = CVAR_FLOAT},
     {.name = "console.input_box_margin", .expected = CVAR_FLOAT},
     {.name = "console.input_box_stroke", .expected = CVAR_FLOAT},
     {.name = "console.input_right_pad", .expected = CVAR_FLOAT},
     {.name     = "console.font_size",
      .expected = CVAR_FLOAT,
      .range    = {.has_min = true, .min.f = 1.0f}},
     {.name     = "console.input_gap",
      .expected = CVAR_FLOAT,
      .range    = {.has_min = true, .min.f = 0.0f}},
     {.name = "console.output_text_color", .expected = CVAR_INT},
     {.name = "console.input_text_color", .expected = CVAR_INT},
     {.name = "console.input_box_color", .expected = CVAR_INT},
     {.name = "console.input_box_background_color", .expected = CVAR_INT},
     {.name = "console.background_color", .expected = CVAR_INT},
};

static void console_init_config(cvar_table *cvars) {
    cvar_set_int(cvars, "console.visible_lines", DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES);
    cvar_set_bool(cvars, "console.enabled", DEFAULT_CONFIG_CONSOLE_ENABLED);
    cvar_set_float(cvars, "console.text_pad_x", DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X);
    cvar_set_float(cvars, "console.top_pad", DEFAULT_CONFIG_CONSOLE_TOP_PAD);
    cvar_set_float(cvars, "console.input_box_margin",
                   DEFAULT_CONFIG_CONSOLE_INPUT_BOX_MARGIN);
    cvar_set_float(cvars, "console.input_box_stroke",
                   DEFAULT_CONFIG_CONSOLE_INPUT_BOX_STROKE);
    cvar_set_float(cvars, "console.input_right_pad",
                   DEFAULT_CONFIG_CONSOLE_INPUT_RIGHT_PAD);
    cvar_set_float(cvars, "console.font_size", DEFAULT_CONFIG_CONSOLE_FONT_SIZE);
    cvar_set_float(cvars, "console.input_gap", DEFAULT_CONFIG_CONSOLE_INPUT_GAP);
    cvar_set_int(cvars, "console.output_text_color",
                 DEFAULT_CONFIG_CONSOLE_OUTPUT_TEXT_COLOR);
    cvar_set_int(cvars, "console.input_text_color",
                 DEFAULT_CONFIG_CONSOLE_INPUT_TEXT_COLOR);
    cvar_set_int(cvars, "console.input_box_color",
                 DEFAULT_CONFIG_CONSOLE_INPUT_BOX_COLOR);
    cvar_set_int(cvars, "console.input_box_background_color",
                 DEFAULT_CONFIG_CONSOLE_INPUT_BOX_BACKGROUND_COLOR);
    cvar_set_int(cvars, "console.background_color",
                 DEFAULT_CONFIG_CONSOLE_BACKGROUND_COLOR);

    cvar_add_schema(cvars, g_console_constraints,
                    sizeof(g_console_constraints) / sizeof(g_console_constraints[0]));
}

void console_ext_install(void) {
    zod_register_extension((zod_extension){
         .init_config  = console_init_config,
         .apply_config = console_apply_config,
    });
}

void console_apply_config(void) {
    g_console.visible_lines = cvar_get_int(&g_ctx.config.cvars, "console.visible_lines",
                                           DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES);
    g_console.enabled       = cvar_get_bool(&g_ctx.config.cvars, "console.enabled",
                                            DEFAULT_CONFIG_CONSOLE_ENABLED);
    g_console.text_pad_x = cvar_get_float(&g_ctx.config.cvars, "console.text_pad_x",
                                          DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X);
    g_console.top_pad    = cvar_get_float(&g_ctx.config.cvars, "console.top_pad",
                                          DEFAULT_CONFIG_CONSOLE_TOP_PAD);
    g_console.input_box_margin =
         cvar_get_float(&g_ctx.config.cvars, "console.input_box_margin",
                        DEFAULT_CONFIG_CONSOLE_INPUT_BOX_MARGIN);
    g_console.input_box_stroke =
         cvar_get_float(&g_ctx.config.cvars, "console.input_box_stroke",
                        DEFAULT_CONFIG_CONSOLE_INPUT_BOX_STROKE);
    g_console.input_right_pad =
         cvar_get_float(&g_ctx.config.cvars, "console.input_right_pad",
                        DEFAULT_CONFIG_CONSOLE_INPUT_RIGHT_PAD);
    g_console.font_size = cvar_get_float(&g_ctx.config.cvars, "console.font_size",
                                         DEFAULT_CONFIG_CONSOLE_FONT_SIZE);
    g_console.input_gap = cvar_get_float(&g_ctx.config.cvars, "console.input_gap",
                                         DEFAULT_CONFIG_CONSOLE_INPUT_GAP);
    g_console.output_text_color = color4f_from_u32(
         (uint32_t)cvar_get_int(&g_ctx.config.cvars, "console.output_text_color",
                                DEFAULT_CONFIG_CONSOLE_OUTPUT_TEXT_COLOR));
    g_console.input_text_color = color4f_from_u32(
         (uint32_t)cvar_get_int(&g_ctx.config.cvars, "console.input_text_color",
                                DEFAULT_CONFIG_CONSOLE_INPUT_TEXT_COLOR));
    g_console.input_box_color = color4f_from_u32(
         (uint32_t)cvar_get_int(&g_ctx.config.cvars, "console.input_box_color",
                                DEFAULT_CONFIG_CONSOLE_INPUT_BOX_COLOR));
    g_console.input_box_background_color = color4f_from_u32((uint32_t)cvar_get_int(
         &g_ctx.config.cvars, "console.input_box_background_color",
         DEFAULT_CONFIG_CONSOLE_INPUT_BOX_BACKGROUND_COLOR));
    g_console.background_color = color4f_from_u32(
         (uint32_t)cvar_get_int(&g_ctx.config.cvars, "console.background_color",
                                DEFAULT_CONFIG_CONSOLE_BACKGROUND_COLOR));
}

bool console_toggle(void) {
    if (!g_console.enabled) return g_console.visible;

    g_console.visible = !g_console.visible;

    if (g_ctx.window.handle) {
        if (g_console.visible) {
            SDL_StartTextInput(g_ctx.window.handle);
        } else {
            SDL_StopTextInput(g_ctx.window.handle);
        }
    }

    return g_console.visible;
}

bool console_visible(void) { return g_console.visible; }

void console_handle_event(console_input_event event) {
    if (!g_console.visible) return;

    switch (event.kind) {
        case CONSOLE_INPUT_TEXT:
            for (const char *p = event.text; p && *p; p++) console_input_append(*p);
            break;
        case CONSOLE_INPUT_BACKSPACE:
            console_input_backspace();
            break;
        case CONSOLE_INPUT_SUBMIT:
            console_input_submit();
            break;
        case CONSOLE_INPUT_LEFT:
            console_input_move_left();
            break;
        case CONSOLE_INPUT_RIGHT:
            console_input_move_right();
            break;
    }
}

static void console_write_v(const char *fmt, va_list args) {
    if (g_console.count == CONSOLE_MAX_LINES) {
        memmove(g_console.lines[0], g_console.lines[1],
                (size_t)(CONSOLE_MAX_LINES - 1) * CONSOLE_MAX_LINE_LEN);
        g_console.count--;
    }

    vsnprintf(g_console.lines[g_console.count], CONSOLE_MAX_LINE_LEN, fmt, args);
    g_console.count++;
}

void console_write(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    console_write_v(fmt, args);
    va_end(args);
}

int console_panel_height(int window_height, int visible_lines, int row_height,
                         int overhead) {
    int desired = visible_lines * row_height + overhead;
    return desired < window_height ? desired : window_height;
}

int console_visible_line_start(int count, int lines_that_fit) {
    int start = count - lines_that_fit;
    return start > 0 ? start : 0;
}

void console_input_append(char c) {
    if (g_console.input_len >= CONSOLE_INPUT_MAX_LEN - 1) return;
    memmove(&g_console.input[g_console.cursor_pos + 1],
            &g_console.input[g_console.cursor_pos],
            (size_t)(g_console.input_len - g_console.cursor_pos) + 1);
    g_console.input[g_console.cursor_pos] = c;
    g_console.input_len++;
    g_console.cursor_pos++;
}

void console_input_backspace(void) {
    if (g_console.cursor_pos == 0) return;
    memmove(&g_console.input[g_console.cursor_pos - 1],
            &g_console.input[g_console.cursor_pos],
            (size_t)(g_console.input_len - g_console.cursor_pos) + 1);
    g_console.input_len--;
    g_console.cursor_pos--;
}

void console_input_move_left(void) {
    if (g_console.cursor_pos > 0) g_console.cursor_pos--;
}

void console_input_move_right(void) {
    if (g_console.cursor_pos < g_console.input_len) g_console.cursor_pos++;
}

void console_input_submit(void) {
    if (g_console.input_len == 0) return;
    console_write("%s", g_console.input);
    g_console.input[0]   = '\0';
    g_console.input_len  = 0;
    g_console.cursor_pos = 0;
}

bool console_draw(void) {
    if (!g_console.visible) return true;

    int row_height = (int)(g_console.font_size * CONSOLE_LINE_HEIGHT_RATIO);
    int overhead   = (int)(g_console.top_pad + g_console.input_gap);
    // +1 reserves a row for the input line so visible_lines rows of
    // scrollback actually fit — console_platform_draw's lines_fit already
    // budgets one row for input (scrollback_rows = lines_fit - 1).
    int height = console_panel_height(g_ctx.window.height, g_console.visible_lines + 1,
                                      row_height, overhead);
    console_platform_draw(g_ctx.window.width, height);
    return true;
}

bool console_destroy(void) { return true; }

#else  

void console_ext_install(void) {}
void console_apply_config(void) {}
bool console_toggle(void) { return false; }
bool console_visible(void) { return false; }
void console_handle_event(console_input_event event) { (void)event; }
void console_write(const char *fmt, ...) { (void)fmt; }
bool console_draw(void) { return true; }
bool console_destroy(void) { return true; }

#endif  

#endif  // ZOD_NGINE_IMPLEMENTATION
