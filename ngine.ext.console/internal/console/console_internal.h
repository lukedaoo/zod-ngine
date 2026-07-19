#ifndef CONSOLE_INTERNAL_H
#define CONSOLE_INTERNAL_H

#include <stdarg.h>

#include <ngine.lib/cvar.h>
#include <ngine.lib/types.h>

#ifndef ZOD_CONSOLE_ENABLE
#define ZOD_CONSOLE_ENABLE 1
#endif

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

#ifndef DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES
#define DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES 10
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_ENABLED
#define DEFAULT_CONFIG_CONSOLE_ENABLED false
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X
#define DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X 4.0f
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_TOP_PAD
#define DEFAULT_CONFIG_CONSOLE_TOP_PAD 10.0f
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_BOX_MARGIN
#define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_MARGIN 4.0f
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_BOX_STROKE
#define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_STROKE 1.0f
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_RIGHT_PAD
#define DEFAULT_CONFIG_CONSOLE_INPUT_RIGHT_PAD 8.0f
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_OUTPUT_TEXT_COLOR
#define DEFAULT_CONFIG_CONSOLE_OUTPUT_TEXT_COLOR 0xFFFFFFFF
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_TEXT_COLOR
#define DEFAULT_CONFIG_CONSOLE_INPUT_TEXT_COLOR 0xFFFFFFFF
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_BOX_COLOR
#define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_COLOR 0xFFFFFF66
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_BACKGROUND_COLOR
#define DEFAULT_CONFIG_CONSOLE_BACKGROUND_COLOR 0x000000D9
#endif

typedef struct console_state {
    bool enabled;
    bool visible;
    int  count;
    char lines[CONSOLE_MAX_LINES][CONSOLE_MAX_LINE_LEN];
    char input[CONSOLE_INPUT_MAX_LEN];
    int  input_len;
    int  cursor_pos;

    float   text_pad_x;
    float   top_pad;
    float   input_box_margin;
    float   input_box_stroke;
    float   input_right_pad;
    color4f output_text_color;
    color4f input_text_color;
    color4f input_box_color;
    color4f background_color;
} console_state;

#if ZOD_CONSOLE_ENABLE
static console_state g_console;
#endif

// Seeds console.* defaults into `cvars` and registers console's own cvar
// constraints against it — the zod_extension.init_config hook. Runs once at
// engine init (before the config file loads) and again on every hot-reload
// (into the temp table being rebuilt), so it must not assume a global.
void console_init_config(cvar_table *cvars);

// Caches console.enabled from cvars into g_console.enabled — read once here
// instead of looking the cvar up on every grave-key press. Called at engine
// init and again on config hot-reload, same as window_apply_config.
void console_apply_config(void);

int console_panel_height(int window_height, int visible_lines);

// Reads the console.visible_lines cvar (falls back to
// DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES). Kept separate from console_draw
// so it's testable without touching GL.
int console_resolve_visible_lines(void);

// First index of g_console.lines to draw, given how many lines fit in the
// panel — clips to the most recent lines_that_fit entries.
int console_visible_line_start(int count, int lines_that_fit);

// Separated from console_write so a caller already holding a va_list (e.g.
// its own variadic wrapper) can hand it on without re-packing varargs.
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
