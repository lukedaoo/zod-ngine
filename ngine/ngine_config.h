#ifndef NGINE_CONFIG_H
#define NGINE_CONFIG_H

// Engine defaults. Included automatically by index.h —
// do not include manually. Uncomment a line to override its default.

// ------- Clock
// config.c — target frames per second
// @default 60
// #define DEFAULT_CONFIG_TARGET_FPS 60

// config.c — minimum frames per second
// @default 0.25
// #define DEFAULT_CONFIG_CLOCK_STAMP 0.25F

// ------- Window
// config.c — initial window width in pixels
// @default 800
// #define DEFAULT_CONFIG_WINDOW_WIDTH 800

// config.c — initial window height in pixels
// @default 600
// #define DEFAULT_CONFIG_WINDOW_HEIGHT 600

// config.c — window title string
// @default "zod-ngine"
// #define DEFAULT_CONFIG_WINDOW_TITLE "zod-ngine"

// config.c — enable vertical sync
// @default true
// #define DEFAULT_CONFIG_WINDOW_VSYNC true

// config.c — OS-level window compositing transparency (fixed at creation, no runtime
// toggle)
// @default false
// #define DEFAULT_CONFIG_WINDOW_TRANSPARENT false

// config.c — clear color packed as 0xRRGGBBAA
// @default 0xFF141A1A  (~0.08r 0.10g 0.10b 1.0a — dark teal-gray)
// #define DEFAULT_CONFIG_WINDOW_CLEAR_COLOR 0x141A1AFF

// ------- Console
// console_internal.h — compile-time inclusion of the console module. Normally
// set via build.c (release builds pass -DZOD_CONSOLE_ENABLE=0 so console code
// and shader strings never ship in the binary); this override only matters
// when compiling without going through build.c.
// @default 1
// #define ZOD_CONSOLE_ENABLE 1

// config.c — number of buffered lines shown in the console panel
// @default 10
// #define DEFAULT_CONFIG_CONSOLE_VISIBLE_LINES 10

// config.c — console starts enabled (grave key toggle armed). Runtime-only
// gate — set true here or in engine.scf for a dev build, keep false for a
// shipped build even when ZOD_CONSOLE_ENABLE keeps the code compiled in.
// @default false
// #define DEFAULT_CONFIG_CONSOLE_ENABLED false

// config.c — left inset for scrollback/input text
// @default 4.0
// #define DEFAULT_CONFIG_CONSOLE_TEXT_PAD_X 4.0f

// config.c — clearance above the first scrollback row
// @default 10.0
// #define DEFAULT_CONFIG_CONSOLE_TOP_PAD 10.0f

// config.c — input box inset from the left/right panel edges
// @default 4.0
// #define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_MARGIN 4.0f

// config.c — input box border thickness in px
// @default 1.0
// #define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_STROKE 1.0f

// config.c — right clearance before input text scrolls
// @default 8.0
// #define DEFAULT_CONFIG_CONSOLE_INPUT_RIGHT_PAD 8.0f

// config.c — scrollback line color packed as 0xRRGGBBAA
// @default 0xFFFFFFFF (opaque white)
// #define DEFAULT_CONFIG_CONSOLE_OUTPUT_TEXT_COLOR 0xFFFFFFFF

// config.c — input line + cursor color packed as 0xRRGGBBAA
// @default 0xFFFFFFFF (opaque white)
// #define DEFAULT_CONFIG_CONSOLE_INPUT_TEXT_COLOR 0xFFFFFFFF

// config.c — input box border color packed as 0xRRGGBBAA
// @default 0xFFFFFF66 (white, ~0.4 alpha)
// #define DEFAULT_CONFIG_CONSOLE_INPUT_BOX_COLOR 0xFFFFFF66

// config.c — panel background color packed as 0xRRGGBBAA
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

// ------- Render
// render_internal.h — compile-time backend selection (0 = OpenGL, 1 = Vulkan).
// Normally set via `./nob run engine --backend=opengl|vulkan`; this override
// only matters when compiling without going through build.c (e.g. a raw cc
// invocation or an IDE that doesn't pass -DRENDER_BACKEND).
// @default 0 (RENDER_BACKEND_OPENGL)
// #define RENDER_BACKEND 0

// ------- Log
// config.c — initial log level (0=TRACE 1=DEBUG 2=INFO 3=WARN 4=ERROR 5=FATAL)
// @default 0
// #define DEFAULT_CONFIG_LOG_LEVEL 0

#endif
