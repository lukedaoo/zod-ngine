#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>
#include <ngine.ext.console/index.h>

#include "beatup_gameplay.c"
#include "beatup_hud.c"

int beatup_run(int argc, const char **argv) {
    log_debug("beatup: starting");
    const zngine_dispatch dispatch = {.load_args = load_args};

    const zngine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path      = CONFIG_PATH,
                          .hot_reload       = true,
                          .load_config_func = load_config_from_file_default},
         .dispatch     = dispatch,
    };

#if ZOD_CONSOLE_ENABLED
    zconsole_ext_install();
#endif

    if (!zngine_init(params)) return 1;
    beatup_actions_bind();
    zngine_command_register(COMMAND_GROUP_USER_DEFINED, "autoplay", beatup_cmd_autoplay);
    zngine_keybind_manager_load(KEYBINDING_PATH, &beatup_vocab);
    render_text_init();
    render_sprite_init();
    beatup_layout_init();
    beatup_song_load(SONG_PATH);
    beatup_music_load(MUSIC_PATH);
    beatup_sfx_load();

    while (!zngine_should_exit()) {
        zngine_clock_update();
        beatup_song_time_update();
        beatup_countdown_update();
        beatup_gameplay_update(zngine_clock_dt());
        beatup_autoplay_update();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zngine_request_exit();
            if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                sys_event_resize_payload payload = {.width  = e.window.data1,
                                                    .height = e.window.data2};
                event_context            ctx     = {
                     .identifier   = {.category = EVENT_CATEGORY_SYSTEM,
                                      .event_id = SYS_EVENT_ON_RESIZE_WINDOW},
                     .payload      = &payload,
                     .payload_size = sizeof(payload)};
                zngine_event_publish(&ctx);
            }
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                const action_handle handle =
                     zngine_keybind_resolve(beatup_active_context(), (int)e.key.scancode,
                                            ACTION_TRIGGER_KEY_PRESSED);
                if (handle != ACTION_HANDLE_INVALID) zngine_action_execute(handle, NULL);
            }
            zngine_extensions_handle_event(&e);
        }

        zngine_input_update();
        zngine_tick_hot_reload();

        zngine_begin_drawing();
        beatup_layout_draw();
        beatup_hud_draw();

        render_text_flush();
        render_sprite_flush();
        zngine_extensions_draw();
        zngine_end_drawing();
        zngine_clock_sleep_to_target_fps();
    }

    beatup_music_destroy();
    beatup_layout_destroy();
    render_sprite_destroy();
    render_text_destroy();
    zngine_destroy();
    return 0;
}
