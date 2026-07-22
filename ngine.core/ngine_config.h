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

// ------- Extensions
// zod_ngine.c — max extensions zngine_register_extension can hold (fixed-size
// array, no dynamic allocation)
// @default 8
// #define ZOD_MAX_EXTENSIONS 8

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
