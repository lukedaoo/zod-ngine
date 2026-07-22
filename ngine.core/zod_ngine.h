#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <ngine.lib/cvar.h>
#include <ngine.lib/cvar_load.h>
#include <ngine.lib/simple_font.h>
#include <ngine.lib/command.h>

#include "input.h"

typedef struct {
    void (*before_init)(void *user_data);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void *user_data);

    void (*before_destroy)(void *user_data);
    void (*after_destroy)(void *user_data);
} zngine_dispatch;

typedef struct {
    void (*init_config)(cvar_table *cvars);
    void (*apply_config)(void);
} zngine_extension;

// Must be called before zngine_init() so init_config runs in time to
// register the extension's constraints before the config file loads.
void zngine_register_extension(zngine_extension ext);

// Runs every registered extension's init_config against `cvars`. Called by
// zngine_init and by config_reload_from_file (config.c) — not meant for
// application code.
void zngine_run_extension_init_config(cvar_table *cvars);

typedef struct {
    const char            *config_path;
    bool                   hot_reload;
    const cvar_constraint *constraints;
    size_t                 constraints_count;
    bool (*load_config_func)(const char *filepath, cvar_table *cvars);
} zngine_config_setup;

typedef struct {
    int          argc;
    const char **argv;

    zngine_config_setup config_setup;

    zngine_dispatch dispatch;

    void *user_data;
} zngine_init_params;

//
// Engine lifecycle
//
bool zngine_init(const zngine_init_params params);
void zngine_destroy(void);

void zngine_apply_config(bool adjust_config);

// Call from the app's event loop on SDL_EVENT_WINDOW_RESIZED so the
// viewport tracks interactive drag-resizes (only fires when window.resizable
// is set).
void zngine_window_notify_resized(int width, int height);

//
// Config accessors
//
int         zngine_config_get_int(const char *name, int fallback);
float       zngine_config_get_float(const char *name, float fallback);
bool        zngine_config_get_bool(const char *name, bool fallback);
const char *zngine_config_get_string(const char *name, const char *fallback);
bool        zngine_config_set_int(const char *name, int value);
bool        zngine_config_set_float(const char *name, float value);
bool        zngine_config_set_bool(const char *name, bool value);
bool        zngine_config_set_string(const char *name, const char *value);

//
// Clock accessors
//

// @info(clock):
//   dt         — scaled delta time (0 when paused, affected by time_scale)
//   delta      — unscaled delta time (always ticks, use for UI/menus)
//   time_scale — multiplier applied to delta: 1.0 = normal, 0.5 = half speed
//   frame_time — elapsed time of the current frame (unscaled, now - last)
float    zngine_clock_dt(void);
float    zngine_clock_delta(void);
double   zngine_clock_now(void);
float    zngine_clock_frame_time(void);
uint32_t zngine_clock_frame(void);
bool     zngine_clock_paused(void);
void     zngine_clock_set_time_scale(float scale);
void     zngine_clock_set_paused(bool paused);
void     zngine_clock_update(void);
void     zngine_clock_sleep_to_target_fps(void);

//
// Render accessors
//
void zngine_begin_drawing(void);
void zngine_end_drawing(void);

//
// Font accessors
//

// Borrowed pointer into the engine's own primary_font storage — valid for the
// engine's lifetime, do not free. Reflects whatever was most recently loaded by
// zngine_apply_config's load_font (initial load or config hot-reload).
const simple_font *zngine_font_primary_get(void);

//
// Input accessors
//
void zngine_input_update(void);
bool zngine_input_key_down(zod_key_t key);
bool zngine_input_key_pressed(zod_key_t key);
bool zngine_input_key_released(zod_key_t key);

//
// Command Manager accessors
//
bool zngine_command_register(command_group group, const char *name,
                          command_execute_result (*handler)(int argc, char **argv));
bool zngine_command_unregister(command_group group, const char *name);
command_execute_result zngine_sys_command_execute(const char *name, int argc, char **argv);
command_execute_result zngine_user_command_execute(const char *name, int argc, char **argv);

// Utils
bool zngine_should_exit(void);
void zngine_request_exit(void);

bool zngine_tick_hot_reload(void);

#endif
