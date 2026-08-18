#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL_events.h>
#include <ngine.lib/cvar.h>
#include <ngine.lib/cvar_load.h>
#include <ngine.lib/simple_font.h>
#include <ngine.lib/command.h>
#include <ngine.lib/event_dispatcher.h>
#include <ngine.lib/action.h>
#include <ngine.lib/keybind.h>

#include "input.h"
#include "keybind_manager.h"

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
    bool (*init)(void);
    // @todo(input): takes the engine event wrapper once it exists.
    void (*handle_event)(const SDL_Event *event);
    void (*draw)(void);
    void (*shutdown)(void);
} zngine_extension;

// Must be called before zngine_init() so init_config runs in time to
// register the extension's constraints before the config file loads.
void zngine_register_extension(zngine_extension ext);

// @todo(input): SDL_Event will be replaced by an engine-owned event wrapper —
// this signature and zngine_extension.handle_event change with it.
void zngine_extensions_handle_event(const SDL_Event *event);
void zngine_extensions_draw(void);

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

// Live window size in pixels — reflects the current mode (windowed at any
// requested resolution, or fullscreen at the display's actual resolution),
// updated on resize/fullscreen toggle. Use this instead of hardcoding a
// canvas size so layout code adapts to whatever window.width/height (or
// fullscreen) is actually configured.
int zngine_window_width(void);
int zngine_window_height(void);

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
command_handle zngine_command_register(command_group group, const char *name,
                                       command_execute_result (*handler)(int    argc,
                                                                         char **argv));
bool           zngine_command_unregister(command_group group, const char *name);
command_execute_result zngine_sys_command_execute(const char *name, int argc,
                                                  char **argv);
command_execute_result zngine_user_command_execute(const char *name, int argc,
                                                   char **argv);

//
// Event Manager accessors
//
event_handle zngine_event_subscribe(const event_category category, const int event_id,
                                    const event_callback callback, void *userdata,
                                    const event_userdata_destroy destroy_fn);
bool         zngine_event_unsubscribe(const event_handle handle);
bool         zngine_event_unsubscribe_by_event_identifier(const event_category category,
                                                          const int            event_id);
void         zngine_event_publish(event_context *ctx);
void zngine_event_emit(const event_category category, const int event_id, void *payload,
                       const size_t payload_size);

//
// Action Manager accessors
//
action_handle zngine_action_resolve_by_name(const action_mode         context,
                                            const action_trigger_type type,
                                            const char               *name);
action_handle zngine_action_resolve_by_key(const action_mode         context,
                                           const action_trigger_type type, const int key);

action_handle zngine_action_bind(const action_mode         context,
                                 const action_trigger_type type, const int key,
                                 const char *name, action_executor *executor);
bool zngine_action_rebind(const action_mode context, const action_trigger_type type,
                          const int old_key, const int new_key);

bool zngine_action_unbind_by_key(const action_mode         context,
                                 const action_trigger_type type, const int key);
bool zngine_action_unbind_by_name(const action_mode         context,
                                  const action_trigger_type type, const char *name);
bool zngine_action_unbind_all(const action_mode context, const action_trigger_type type);

size_t zngine_action_dispatch(const action_mode context, void *userdata);

action_execute_result zngine_action_execute(const action_handle handle, void *userdata);
action_execute_result zngine_action_execute_by_name(const action_mode         context,
                                                    const action_trigger_type type,
                                                    const char *name, void *userdata);

//
// Keybind Manager accessors
//
bool          zngine_keybind_manager_load(const char *path, const keybind_vocab *vocab);
bool          zngine_keybind_manager_merge(const char *path, const keybind_vocab *vocab);
bool          zngine_keybind_bind(const action_mode context, const int key,
                                  const int trigger, const char *action_name);
bool          zngine_keybind_unbind(const action_mode context, const int key,
                                    const int trigger);
action_handle zngine_keybind_resolve(const action_mode context, const int key,
                                     const int trigger);

size_t zngine_keybind_count(void);
bool   zngine_keybind_at(const size_t index, keybind_manager_entry *out);

// Utils
bool zngine_should_exit(void);
void zngine_request_exit(void);

bool zngine_tick_hot_reload(void);

#endif
