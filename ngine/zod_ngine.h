#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <modules/cvar.h>

typedef struct {
    void (*before_init)(void);
    bool (*load_args)(int argc, const char **argv, cvar_table *cvars);
    void (*after_init)(void);

    void (*before_destroy)(void);
    void (*after_destroy)(void);
} zod_engine_dispatch;

typedef struct {
    const char *config_path;
    bool        hot_reload;
    bool (*load_config_func)(const char *filepath, cvar_table *cvars);
} zod_config_file_setup_t;

typedef struct {
    int          argc;
    const char **argv;

    zod_config_file_setup_t config_file_setup;

    zod_engine_dispatch dispatch;
} zod_engine_init_params;

//
// Engine lifecycle
//
bool zod_ngine_init(const zod_engine_init_params params);
void zod_ngine_destroy(void);

void zod_ngine_apply_config(void);

//
// Config accessors
//
int         config_get_int(const char *name, int fallback);
float       config_get_float(const char *name, float fallback);
bool        config_get_bool(const char *name, bool fallback);
const char *config_get_string(const char *name, const char *fallback);
bool        config_set_int(const char *name, int value);
bool        config_set_float(const char *name, float value);
bool        config_set_bool(const char *name, bool value);
bool        config_set_string(const char *name, const char *value);

//
// Clock accessors
//

// @info(clock):
//   dt         — scaled delta time (0 when paused, affected by time_scale)
//   delta      — unscaled delta time (always ticks, use for UI/menus)
//   time_scale — multiplier applied to delta: 1.0 = normal, 0.5 = half speed
//   frame_time — elapsed time of the current frame (unscaled, now - frame_last)
float    clock_dt(void);
float    clock_delta(void);
float    clock_now(void);
float    clock_frame_time(void);
uint32_t clock_frame(void);
bool     clock_paused(void);
void     clock_set_time_scale(float scale);
void     clock_set_paused(bool paused);

//
// Render accessors
//
void zod_begin_drawing(void);
void zod_end_drawing(void);

#endif
