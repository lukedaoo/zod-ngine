#include <stdbool.h>

#define ZOD_NGINE_IMPLEMENTATION
#include <ngine.core/index.h>

#define CONFIG_PATH "run-tree/data/keybinding_run.scf"

enum : uint8_t { INPUT_CONTEXT_GAME = 0 } input_contexts;

static const keybind_context keybinding_contexts[] = {
     {.name = "game", .context = INPUT_CONTEXT_GAME},
};

static const keybind_vocab keybinding_vocab = {
     .contexts      = keybinding_contexts,
     .context_count = 1,
     .key_from_name = zngine_key_from_name,
     .key_to_name   = zngine_key_to_name,
};

static action_execute_result say_hello_execute(const action_table *table,
                                               action_handle self, void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    log_info("keybinding_run: hello!");
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static action_execute_result say_world_execute(const action_table *table,
                                               action_handle self, void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    log_info("keybinding_run: world!");
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

static action_execute_result quit_execute(const action_table *table, action_handle self,
                                          void *userdata) {
    (void)table;
    (void)self;
    (void)userdata;
    log_info("keybinding_run: quit");
    zngine_request_exit();
    return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
}

int main(const int argc, const char **argv) {
    log_debug("keybinding_run: starting");
    const zngine_init_params params = {
         .argc         = argc,
         .argv         = argv,
         .config_setup = {.config_path      = CONFIG_PATH,
                          .hot_reload       = true,
                          .load_config_func = load_config_from_file_default},
    };

    if (!zngine_init(params)) return 1;

    action_executor say_hello = {.execute = say_hello_execute};
    action_executor say_world = {.execute = say_world_execute};
    action_executor quit      = {.execute = quit_execute};
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_H,
                       "say_hello", &say_hello);
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED, SDL_SCANCODE_W,
                       "say_world", &say_world);
    zngine_action_bind(INPUT_CONTEXT_GAME, ACTION_TRIGGER_KEY_PRESSED,
                       SDL_SCANCODE_ESCAPE, "quit", &quit);

    if (!zngine_keybind_manager_load(CONFIG_PATH, &keybinding_vocab))
        log_warn("keybinding_run: failed to load keybinds from '%s'", CONFIG_PATH);

    while (!zngine_should_exit()) {
        zngine_clock_update();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) zngine_request_exit();
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
                const action_handle handle = zngine_keybind_resolve(
                     INPUT_CONTEXT_GAME, (int)e.key.scancode, ACTION_TRIGGER_KEY_PRESSED);
                if (handle != ACTION_HANDLE_INVALID) zngine_action_execute(handle, NULL);
            }
        }

        zngine_begin_drawing();
        zngine_end_drawing();
    }

    zngine_destroy();
    return 0;
}
