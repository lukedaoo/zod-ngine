#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "../engine_context/engine_context_internal.h"
#include "../../console.h"
#include "console_internal.h"

bool console_toggle(void) {
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

void console_write_v(const char *fmt, va_list args) {
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

int console_panel_height(int window_height, int visible_lines) {
    int desired = visible_lines * CONSOLE_LINE_HEIGHT;
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

int console_resolve_visible_lines(void) {
    return cvar_get_int(&g_ctx.config.cvars, "console.visible_lines",
                        DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES);
}

bool console_draw(void) {
    if (!g_console.visible) return true;

    int height =
         console_panel_height(g_ctx.window.height, console_resolve_visible_lines());
    console_platform_draw(g_ctx.window.width, height);
    return true;
}

bool console_destroy(void) { return true; }

#endif
