#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <modules/cvar.h>
#include <modules/cvar_load.h>
#include <modules/simple_font.h>

#include "input.h"
#include "console.h"

typedef struct {
    void (*before_init)(void *user_data);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void *user_data);

    void (*before_destroy)(void *user_data);
    void (*after_destroy)(void *user_data);
} zod_engine_dispatch;

typedef struct {
    const char            *config_path;
    bool                   hot_reload;
    const cvar_constraint *constraints;
    size_t                 constraints_count;
    bool (*load_config_func)(const char *filepath, cvar_table *cvars);
} zod_config_setup;

typedef struct {
    int          argc;
    const char **argv;

    zod_config_setup config_setup;

    zod_engine_dispatch dispatch;

    void *user_data;
} zod_engine_init_params;

//
// Engine lifecycle
//
bool zod_ngine_init(const zod_engine_init_params params);
void zod_ngine_destroy(void);

void zod_ngine_apply_config(bool adjust_config);

//
// Config accessors
//
int         zod_config_get_int(const char *name, int fallback);
float       zod_config_get_float(const char *name, float fallback);
bool        zod_config_get_bool(const char *name, bool fallback);
const char *zod_config_get_string(const char *name, const char *fallback);
bool        zod_config_set_int(const char *name, int value);
bool        zod_config_set_float(const char *name, float value);
bool        zod_config_set_bool(const char *name, bool value);
bool        zod_config_set_string(const char *name, const char *value);

//
// Clock accessors
//

// @info(clock):
//   dt         — scaled delta time (0 when paused, affected by time_scale)
//   delta      — unscaled delta time (always ticks, use for UI/menus)
//   time_scale — multiplier applied to delta: 1.0 = normal, 0.5 = half speed
//   frame_time — elapsed time of the current frame (unscaled, now - last)
float    zod_clock_dt(void);
float    zod_clock_delta(void);
double   zod_clock_now(void);
float    zod_clock_frame_time(void);
uint32_t zod_clock_frame(void);
bool     zod_clock_paused(void);
void     zod_clock_set_time_scale(float scale);
void     zod_clock_set_paused(bool paused);
void     zod_clock_update(void);
void     zod_clock_sleep_to_target_fps(void);

//
// Render accessors
//
void zod_begin_drawing(void);
void zod_end_drawing(void);

//
// Font accessors
//

// Borrowed pointer into the engine's own primary_font storage — valid for the
// engine's lifetime, do not free. Reflects whatever was most recently loaded by
// zod_ngine_apply_config's load_font (initial load or config hot-reload).
const simple_font *zod_font_primary_get(void);

//
// Input accessors
//
void zod_input_update(void);
bool zod_input_key_down(zod_key_t key);
bool zod_input_key_pressed(zod_key_t key);
bool zod_input_key_released(zod_key_t key);

//
// Console accessors
//
bool zod_console_toggle(void);
void zod_console_write(const char *fmt, ...);
bool zod_console_draw(void);
bool zod_console_visible(void);
void zod_console_handle_event(console_input_event event);

// Utils
bool zod_should_exit(void);
void zod_request_exit(void);

bool zod_tick_hot_reload(void);

#endif
