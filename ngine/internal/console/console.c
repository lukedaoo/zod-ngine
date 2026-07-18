#ifdef ZOD_NGINE_IMPLEMENTATION

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../engine_context/engine_context_internal.h"
#include "console_internal.h"

bool console_toggle(void) {
    g_console.visible = !g_console.visible;
    return g_console.visible;
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
