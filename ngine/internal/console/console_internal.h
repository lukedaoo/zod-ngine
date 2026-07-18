#ifndef CONSOLE_INTERNAL_H
#define CONSOLE_INTERNAL_H

#include <stdarg.h>

#ifndef CONSOLE_MAX_LINES
#define CONSOLE_MAX_LINES 128
#endif

#ifndef CONSOLE_MAX_LINE_LEN
#define CONSOLE_MAX_LINE_LEN 256
#endif

#ifndef CONSOLE_LINE_HEIGHT
#define CONSOLE_LINE_HEIGHT 20
#endif

#ifndef CONSOLE_INPUT_MAX_LEN
#define CONSOLE_INPUT_MAX_LEN 128
#endif

typedef struct console_state {
    bool visible;
    int  count;
    char lines[CONSOLE_MAX_LINES][CONSOLE_MAX_LINE_LEN];
    char input[CONSOLE_INPUT_MAX_LEN];
    int  input_len;
    int  cursor_pos;
} console_state;

static console_state g_console;

int console_panel_height(int window_height, int visible_lines);

// Reads the console.visible_lines cvar (falls back to
// DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES). Kept separate from console_draw
// so it's testable without touching GL.
int console_resolve_visible_lines(void);

// First index of g_console.lines to draw, given how many lines fit in the
// panel — clips to the most recent lines_that_fit entries.
int console_visible_line_start(int count, int lines_that_fit);

// Shared by console_write and zod_console_write — the only place a
// variadic call can hand its va_list on to a second variadic-shaped sink.
void console_write_v(const char *fmt, va_list args);

// Typed-input buffer with a movable cursor: characters insert at
// cursor_pos, backspace removes the character before cursor_pos.
void console_input_append(char c);
void console_input_backspace(void);
void console_input_move_left(void);
void console_input_move_right(void);

// Echoes the current input buffer as a new output line, then clears it.
// No-op on an empty buffer.
void console_input_submit(void);

// Draws the panel background + buffered lines for a panel of the given
// pixel size. Implemented per-backend (console_platform_gl.c today).
void console_platform_draw(int width, int height);

#endif
