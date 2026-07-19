#ifndef NGINE_EXT_CONSOLE_CONFIG_H
#define NGINE_EXT_CONSOLE_CONFIG_H

// Console extension defaults. Included automatically by index.h —
// do not include manually. Uncomment a line to override its default.

// console_internal.h — compile-time inclusion of the console module. Normally
// set via build.c (release builds pass -DZOD_CONSOLE_ENABLE=0 so console code
// and shader strings never ship in the binary); this override only matters
// when compiling without going through build.c.
// @default 1
// #define ZOD_CONSOLE_ENABLE 1

// console.c — number of buffered lines shown in the console panel
// @default 10
// #define DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES 10

// console.c — console starts enabled (grave key toggle armed). Runtime-only
// gate — set true here or in engine.scf for a dev build, keep false for a
// shipped build even when ZOD_CONSOLE_ENABLE keeps the code compiled in.
// @default false
// #define DEFAULT_CONFIG_CONSOLE_ENABLED false

// console.c — left inset for scrollback/input text
// @default 4.0
// #define DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X 4.0f

// console.c — clearance above the first scrollback row
// @default 10.0
// #define DEFAULT_CONFIG_CONSOLE_TOP_PAD 10.0f

// console.c — input box inset from the left/right panel edges
// @default 4.0
// #define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_MARGIN 4.0f

// console.c — input box border thickness in px
// @default 1.0
// #define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_STROKE 1.0f

// console.c — right clearance before input text scrolls
// @default 8.0
// #define DEFAULT_CONFIG_CONSOLE_INPUT_RIGHT_PAD 8.0f

// console.c — scrollback line color packed as 0xRRGGBBAA
// @default 0xFFFFFFFF (opaque white)
// #define DEFAULT_CONFIG_CONSOLE_OUTPUT_TEXT_COLOR 0xFFFFFFFF

// console.c — input line + cursor color packed as 0xRRGGBBAA
// @default 0xFFFFFFFF (opaque white)
// #define DEFAULT_CONFIG_CONSOLE_INPUT_TEXT_COLOR 0xFFFFFFFF

// console.c — input box border color packed as 0xRRGGBBAA
// @default 0xFFFFFF66 (white, ~0.4 alpha)
// #define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_COLOR 0xFFFFFF66

// console.c — panel background color packed as 0xRRGGBBAA
// @default 0x000000D9 (black, ~0.85 alpha)
// #define DEFAULT_CONFIG_CONSOLE_BACKGROUND_COLOR 0x000000D9

// console_internal.h — max buffered scrollback lines (fixed-size array bound,
// not cvar-backed — changing it resizes console_state and requires a rebuild)
// @default 128
// #define CONSOLE_MAX_LINES 128

// console_internal.h — max chars per scrollback line, including the
// terminator (fixed-size array bound, not cvar-backed)
// @default 256
// #define CONSOLE_MAX_LINE_LEN 256

// console_internal.h — row height in px used for panel layout and the
// scrollback/input grid. Not cvar-backed — console_panel_height (tested pure
// function) bakes this constant into its return value.
// @default 20
// #define CONSOLE_LINE_HEIGHT 20

// console_internal.h — max chars in the input buffer, including the
// terminator (fixed-size array bound, not cvar-backed)
// @default 128
// #define CONSOLE_INPUT_MAX_LEN 128

#endif
