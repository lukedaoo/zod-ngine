#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ngine.lib/log.h>

#include "../../console.h"
#include "ngine.core/internal/engine_context/engine_context_internal.h"
#include "ngine.core/zod_ngine.h"
#include "console_internal.h"
#include "console_cmd_internal.h"

#if ZOD_CONSOLE_ENABLED

static const color4f g_console_log_colors[] = {
     [LOG_TRACE] = COLOR4F_GRAY, [LOG_DEBUG] = COLOR4F_CYAN,
     [LOG_INFO] = COLOR4F_GREEN, [LOG_WARN] = COLOR4F_YELLOW,
     [LOG_ERROR] = COLOR4F_RED,  [LOG_FATAL] = COLOR4F_MAGENTA,
};

void console_priv_log_hook(int level, const char *message) {
    color4f color = (level >= LOG_TRACE && level <= LOG_FATAL)
                         ? g_console_log_colors[level]
                         : g_console.output_text_color;
    console_priv_write_line(color, message);
}

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
    console_cmd_priv_register();

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

void zconsole_ext_install(void) {
    zngine_register_extension((zngine_extension){
         .init_config  = console_init_config,
         .apply_config = console_priv_apply_config,
    });
}

void console_priv_apply_config(void) {
    g_console.visible_lines = cvar_get_int(&g_ctx.config.cvars, "console.visible_lines",
                                           DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES);
    g_console.enabled       = cvar_get_bool(&g_ctx.config.cvars, "console.enabled",
                                            DEFAULT_CONFIG_CONSOLE_ENABLED);
    g_console.text_pad_x    = cvar_get_float(&g_ctx.config.cvars, "console.text_pad_x",
                                             DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X);
    g_console.top_pad       = cvar_get_float(&g_ctx.config.cvars, "console.top_pad",
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
    g_console.font_size         = cvar_get_float(&g_ctx.config.cvars, "console.font_size",
                                                 DEFAULT_CONFIG_CONSOLE_FONT_SIZE);
    g_console.input_gap         = cvar_get_float(&g_ctx.config.cvars, "console.input_gap",
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
    g_console.input_box_background_color = color4f_from_u32(
         (uint32_t)cvar_get_int(&g_ctx.config.cvars, "console.input_box_background_color",
                                DEFAULT_CONFIG_CONSOLE_INPUT_BOX_BACKGROUND_COLOR));
    g_console.background_color = color4f_from_u32(
         (uint32_t)cvar_get_int(&g_ctx.config.cvars, "console.background_color",
                                DEFAULT_CONFIG_CONSOLE_BACKGROUND_COLOR));
}

bool zconsole_toggle(void) {
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

bool zconsole_visible(void) { return g_console.visible; }

void zconsole_handle_event(zconsole_input_event event) {
    if (!g_console.visible) return;

    switch (event.kind) {
        case ZCONSOLE_INPUT_TEXT:
            for (const char *p = event.text; p && *p; p++) console_priv_input_append(*p);
            break;
        case ZCONSOLE_INPUT_BACKSPACE:
            console_priv_input_backspace();
            break;
        case ZCONSOLE_INPUT_SUBMIT:
            console_priv_input_submit();
            break;
        case ZCONSOLE_INPUT_LEFT:
            console_priv_input_move_left();
            break;
        case ZCONSOLE_INPUT_RIGHT:
            console_priv_input_move_right();
            break;
        case ZCONSOLE_INPUT_HISTORY_PREV:
            console_priv_history_prev();
            break;
        case ZCONSOLE_INPUT_HISTORY_NEXT:
            console_priv_history_next();
            break;
        case ZCONSOLE_INPUT_SCROLL_UP:
            console_priv_scroll_up();
            break;
        case ZCONSOLE_INPUT_SCROLL_DOWN:
            console_priv_scroll_down();
            break;
    }
}

static void console_evict_line_if_full(void) {
    if (g_console.count != CONSOLE_MAX_LINES) return;

    memmove(g_console.lines[0], g_console.lines[1],
            (size_t)(CONSOLE_MAX_LINES - 1) * CONSOLE_MAX_LINE_LEN);
    memmove(g_console.lines_color, g_console.lines_color + 1,
            (size_t)(CONSOLE_MAX_LINES - 1) * sizeof(color4f));
    g_console.count--;
}

static void console_write_v(color4f color, const char *fmt, va_list args) {
    console_evict_line_if_full();

    vsnprintf(g_console.lines[g_console.count], CONSOLE_MAX_LINE_LEN, fmt, args);
    g_console.lines_color[g_console.count] = color;
    g_console.count++;
    g_console.scroll_offset = 0;  // new output snaps the view back to the bottom
}

// text is written verbatim (no % expansion) — safe for log messages that may
// contain literal '%' characters.
void console_priv_write_line(color4f color, const char *text) {
    console_evict_line_if_full();

    snprintf(g_console.lines[g_console.count], CONSOLE_MAX_LINE_LEN, "%s", text);
    g_console.lines_color[g_console.count] = color;
    g_console.count++;
    g_console.scroll_offset = 0;  // new output snaps the view back to the bottom
}

void console_priv_write_multiple_lines(color4f color, const char *text) {
    const char *start = text;
    for (const char *p = text;; p++) {
        if (*p == '\n' || *p == '\0') {
            const char *seg     = start;
            size_t      seg_len = (size_t)(p - start);
            do {
                size_t chunk_len = seg_len < CONSOLE_MAX_LINE_LEN - 1
                                        ? seg_len
                                        : CONSOLE_MAX_LINE_LEN - 1;
                char   buf[CONSOLE_MAX_LINE_LEN];
                memcpy(buf, seg, chunk_len);
                buf[chunk_len] = '\0';
                console_priv_write_line(color, buf);
                seg += chunk_len;
                seg_len -= chunk_len;
            } while (seg_len > 0);  // wrap segments longer than one console line
            if (*p == '\0') break;
            start = p + 1;
        }
    }
}

void zconsole_write(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    console_write_v(g_console.output_text_color, fmt, args);
    va_end(args);
}

void zconsole_write_color(color4f color, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    console_write_v(color, fmt, args);
    va_end(args);
}

int console_priv_panel_height(int window_height, int visible_lines, int row_height,
                              int overhead) {
    int desired = visible_lines * row_height + overhead;
    return desired < window_height ? desired : window_height;
}

int console_priv_visible_line_start(int count, int lines_that_fit, int scroll_offset) {
    int max_offset = count - lines_that_fit;
    if (max_offset < 0) max_offset = 0;
    if (scroll_offset > max_offset) scroll_offset = max_offset;
    if (scroll_offset < 0) scroll_offset = 0;

    int start = count - lines_that_fit - scroll_offset;
    return start > 0 ? start : 0;
}

void console_priv_scroll_up(void) {
    int step = g_console.visible_lines > 0 ? g_console.visible_lines : 1;
    g_console.scroll_offset += step;
    if (g_console.scroll_offset > g_console.count) g_console.scroll_offset = g_console.count;
}

void console_priv_scroll_down(void) {
    int step = g_console.visible_lines > 0 ? g_console.visible_lines : 1;
    g_console.scroll_offset -= step;
    if (g_console.scroll_offset < 0) g_console.scroll_offset = 0;
}

void console_priv_input_append(char c) {
    if (g_console.input_len >= CONSOLE_INPUT_MAX_LEN - 1) return;
    memmove(&g_console.input[g_console.cursor_pos + 1],
            &g_console.input[g_console.cursor_pos],
            (size_t)(g_console.input_len - g_console.cursor_pos) + 1);
    g_console.input[g_console.cursor_pos] = c;
    g_console.input_len++;
    g_console.cursor_pos++;
}

void console_priv_input_backspace(void) {
    if (g_console.cursor_pos == 0) return;
    memmove(&g_console.input[g_console.cursor_pos - 1],
            &g_console.input[g_console.cursor_pos],
            (size_t)(g_console.input_len - g_console.cursor_pos) + 1);
    g_console.input_len--;
    g_console.cursor_pos--;
}

void console_priv_input_move_left(void) {
    if (g_console.cursor_pos > 0) g_console.cursor_pos--;
}

void console_priv_input_move_right(void) {
    if (g_console.cursor_pos < g_console.input_len) g_console.cursor_pos++;
}

void console_priv_history_push(const char *text) {
    if (!text || !*text) return;
    snprintf(g_console.history[g_console.history_next_write], CONSOLE_INPUT_MAX_LEN, "%s",
             text);
    g_console.history_next_write =
         (g_console.history_next_write + 1) % CONSOLE_MAX_HISTORY;
    if (g_console.history_count < CONSOLE_MAX_HISTORY) g_console.history_count++;
    g_console.history_index = 0;
}

// index is 1-based from most recent (1 = newest), matching history_index's convention.
static void console_history_load(int index) {
    int offset = index - 1;
    int idx    = ((g_console.history_next_write - 1 - offset) % CONSOLE_MAX_HISTORY +
                  CONSOLE_MAX_HISTORY) %
                 CONSOLE_MAX_HISTORY;
    snprintf(g_console.input, CONSOLE_INPUT_MAX_LEN, "%s", g_console.history[idx]);
    g_console.input_len  = (int)strlen(g_console.input);
    g_console.cursor_pos = g_console.input_len;
}

void console_priv_history_prev(void) {
    if (g_console.history_count == 0) return;
    if (g_console.history_index == 0) {
        snprintf(g_console.history_draft, CONSOLE_INPUT_MAX_LEN, "%s", g_console.input);
        g_console.history_index = 1;
    } else if (g_console.history_index < g_console.history_count) {
        g_console.history_index++;
    }
    console_history_load(g_console.history_index);
}

void console_priv_history_next(void) {
    if (g_console.history_index == 0) return;
    if (g_console.history_index == 1) {
        g_console.history_index = 0;
        snprintf(g_console.input, CONSOLE_INPUT_MAX_LEN, "%s", g_console.history_draft);
        g_console.input_len  = (int)strlen(g_console.input);
        g_console.cursor_pos = g_console.input_len;
        return;
    }
    g_console.history_index--;
    console_history_load(g_console.history_index);
}

void console_priv_input_submit(void) {
    if (g_console.input_len == 0) return;

    console_priv_history_push(g_console.input);

    //
    // tokenize
    //
    char *argv[COMMAND_MAX_ARGC];
    int   argc = 0;
    char *name = strtok(g_console.input, " ");
    char *tok;
    while (argc < COMMAND_MAX_ARGC && (tok = strtok(NULL, " "))) argv[argc++] = tok;

    // user-defined commands can shadow system ones — try user first.
    command_execute_result result = zngine_user_command_execute(name, argc, argv);
    if (result.type == COMMAND_RESULT_COMMAND_NOT_FOUND)
        result = zngine_sys_command_execute(name, argc, argv);

    if (result.type == COMMAND_RESULT_ERROR) {
        zconsole_write_color(COLOR4F_RED, "ERROR: %s", result.value.str);
    } else if (result.type == COMMAND_RESULT_COMMAND_NOT_FOUND) {
        zconsole_write_color(COLOR4F_RED, "ERROR: invalid command: %s", name);
    } else if (result.type == COMMAND_RESULT_STRING) {
        console_priv_write_multiple_lines(g_console.output_text_color, result.value.str);
    }
    g_console.input[0]   = '\0';
    g_console.input_len  = 0;
    g_console.cursor_pos = 0;
}

bool zconsole_draw(void) {
    if (!g_console.visible) return true;

    int row_height = (int)(g_console.font_size * CONSOLE_LINE_HEIGHT_RATIO);
    int overhead   = (int)(g_console.top_pad + g_console.input_gap);
    // +1 reserves a row for the input line so visible_lines rows of
    // scrollback actually fit — console_priv_platform_draw's lines_fit already
    // budgets one row for input (scrollback_rows = lines_fit - 1).
    int height = console_priv_panel_height(
         g_ctx.window.height, g_console.visible_lines + 1, row_height, overhead);
    console_priv_platform_draw(g_ctx.window.width, height);
    return true;
}

bool zconsole_destroy(void) {
    log_unregister_hook(console_priv_log_hook);
    return true;
}

void console_priv_clear() {
    g_console.count         = 0;
    g_console.cursor_pos    = 0;
    g_console.input_len     = 0;
    g_console.input[0]      = '\0';
    g_console.scroll_offset = 0;

    for (int i = 0; i < CONSOLE_MAX_LINES; i++) {
        g_console.lines[i][0] = '\0';
    }
}

#else

void zconsole_ext_install(void) {}
void console_priv_apply_config(void) {}
bool zconsole_toggle(void) { return false; }
bool zconsole_visible(void) { return false; }
void zconsole_handle_event(zconsole_input_event event) { (void)event; }
void zconsole_write(const char *fmt, ...) { (void)fmt; }
void zconsole_write_color(color4f color, const char *fmt, ...) {
    (void)color;
    (void)fmt;
}
bool zconsole_draw(void) { return true; }
bool zconsole_destroy(void) { return true; }

#endif

#endif  // ZOD_NGINE_IMPLEMENTATION
