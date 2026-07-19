#ifndef CONSOLE_INTERNAL_H
#define CONSOLE_INTERNAL_H

#include <stdarg.h>

#include <ngine.lib/types.h>

#ifndef ZOD_CONSOLE_ENABLED
#define ZOD_CONSOLE_ENABLED 1
#endif

#ifndef CONSOLE_MAX_LINES
#define CONSOLE_MAX_LINES 128
#endif

#ifndef CONSOLE_MAX_LINE_LEN
#define CONSOLE_MAX_LINE_LEN 256
#endif

#ifndef CONSOLE_LINE_HEIGHT_RATIO
#define CONSOLE_LINE_HEIGHT_RATIO 1.25f
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

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_GAP
#define DEFAULT_CONFIG_CONSOLE_INPUT_GAP 8.0f
#endif

#ifndef DEFAULT_CONFIG_CONSOLE_FONT_SIZE
#define DEFAULT_CONFIG_CONSOLE_FONT_SIZE 16.0f
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

#ifndef DEFAULT_CONFIG_CONSOLE_INPUT_BOX_BACKGROUND_COLOR
#define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_BACKGROUND_COLOR 0x00000000
#endif

typedef struct console_state {
    bool enabled;
    bool visible;
    int  count;
    int  visible_lines;
    char lines[CONSOLE_MAX_LINES][CONSOLE_MAX_LINE_LEN];
    char input[CONSOLE_INPUT_MAX_LEN];
    int  input_len;
    int  cursor_pos;

    float   text_pad_x;
    float   top_pad;
    float   input_box_margin;
    float   input_box_stroke;
    float   input_right_pad;
    float   input_gap;
    float   font_size;
    color4f output_text_color;
    color4f input_text_color;
    color4f input_box_color;
    color4f input_box_background_color;
    color4f background_color;
} console_state;

#if ZOD_CONSOLE_ENABLED
static console_state g_console;
#endif

// Caches console.enabled from cvars into g_console.enabled — read once here
// instead of looking the cvar up on every grave-key press. Called at engine
// init and again on config hot-reload, same as window_apply_config.
void console_apply_config(void);

// row_height is the per-line pitch in px (font_size * CONSOLE_LINE_HEIGHT_RATIO).
// overhead is the fixed chrome outside the row grid (top_pad + input_gap) —
// must be added here too since console_platform_draw subtracts the same
// overhead before dividing by row_height, or visible_lines rows wouldn't
// actually fit in the returned height.
int console_panel_height(int window_height, int visible_lines, int row_height,
                         int overhead);

// First index of g_console.lines to draw, given how many lines fit in the
// panel — clips to the most recent lines_that_fit entries.
int console_visible_line_start(int count, int lines_that_fit);

void console_input_append(char c);
void console_input_backspace(void);
void console_input_move_left(void);
void console_input_move_right(void);
void console_input_submit(void);

void console_platform_draw(int width, int height);

#endif
