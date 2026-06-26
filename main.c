#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <stdio.h>

#if DEBUG
#define MODULE_LOG_ENABLED
#endif

#define CARG_IMPLEMENTATION
#define INI_IMPLEMENTATION
#define SCF_IMPLEMENTATION
#define CVAR_IMPLEMENTATION
#define LOG_IMPLEMENTATION
#define LOG_USE_SIMPLE
#define LOG_USE_COLOR
#define CVAR_LOAD_IMPLEMENTATION
#define FILE_WATCHER_IMPLEMENTATION
#include "modules/index.h"

#define RUN_TREE_DIR "run-tree"

struct Window {
    uint32_t      width;
    uint32_t      height;
    SDL_Window   *window;
    SDL_Renderer *renderer;
};

typedef struct {
    cvar_t *version;
    cvar_t *name;
    cvar_t *email;
    cvar_t *password;
    cvar_t *float_var;
    cvar_t *debug_log;
    cvar_t *screen_width;
    cvar_t *screen_height;
    cvar_t *client_name;
    cvar_t *client_email;
    cvar_t *client_password;
} configuration;

int main(const int argc, const char *argv[]) {
    carg_register_t defs[] = {
         {.flag = "--debug-log", .arg_count = 0, .type = CARG_BOOL, .required = false},
         {.flag = "--size", .arg_count = 2, .type = CARG_INT, .required = false},
    };

    carg_table cargs = {0};

    if (!carg_parse(defs, 2, argc, argv, &cargs)) {
        return 1;
    }

    size_t     count_size_arg = 0;
    const int *screen_size    = carg_get_int_array(&cargs, "--size", &count_size_arg);
    if (count_size_arg == 2 && screen_size) {
        log_debug("screen size: %dx%d", screen_size[0], screen_size[1]);
    }

    bool debug_log = carg_get_bool(&cargs, "--debug-log", false);
    if (debug_log) {
        log_set_level(LOG_DEBUG);
    }

    cvar_table cvars = {0};
    FILE      *fp    = fopen("log.txt", "w");
    if (fp) {
        log_set_fp(fp, LOG_DEBUG);
    }
    log_debug("Hello World");
    log_debug("Hello World");
    log_debug("Hello World");
    log_debug("Hello World");
    log_debug("Hello World");

    if (!cvar_load_scf(&cvars, RUN_TREE_DIR "/data/test.scf",
                       cvar_default_config_parser_handler, false)) {
        return 1;
    }

    const char *debug_log_names[] = {"debug_log"};
    const char *size_names[]      = {"screen.width", "screen.height"};

    const char **names_per_carg[]       = {debug_log_names, size_names};
    const size_t names_count_per_carg[] = {1, count_size_arg};

    carg_table_to_cvars(&cargs, names_per_carg, names_count_per_carg, &cvars);

    configuration config = {
         .version         = cvar_get(&cvars, "protocol.version"),
         .name            = cvar_get(&cvars, "user.name"),
         .email           = cvar_get(&cvars, "user.email"),
         .password        = cvar_get(&cvars, "user.password"),
         .float_var       = cvar_get(&cvars, "user.float_var"),
         .debug_log       = cvar_get(&cvars, "debug_log"),
         .screen_width    = cvar_get(&cvars, "screen.width"),
         .screen_height   = cvar_get(&cvars, "screen.height"),
         .client_name     = cvar_get(&cvars, "client.name"),
         .client_email    = cvar_get(&cvars, "client.email"),
         .client_password = cvar_get(&cvars, "client.password"),
    };

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Hello World", 800, 600, 0);
    if (!window) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_SetRenderVSync(renderer, 1);

    SDL_FRect movingRect = {.x = 50, .y = 50, .w = 60, .h = 60};

    uint64_t last_fps_time  = SDL_GetTicksNS();
    uint64_t frame_count    = 0;
    char     fps_buffer[32] = "FPS: 0";

    bool running = true;

    file_watcher *w = file_watcher_watch(RUN_TREE_DIR "/data/test.scf");
    if (!w) return 1;

    while (running) {
        file_status status = file_watcher_check(w);
        if (status == FILE_CHANGED) {
            printf("Configuration changed\n");
            if (cvar_load_scf(&cvars, RUN_TREE_DIR "/data/test.scf",
                              cvar_default_config_parser_handler, false)) {
                config.version       = cvar_get(&cvars, "protocol.version");
                config.name          = cvar_get(&cvars, "user.name");
                config.email         = cvar_get(&cvars, "user.email");
                config.password      = cvar_get(&cvars, "user.password");
                config.float_var     = cvar_get(&cvars, "user.float_var");
                config.debug_log     = cvar_get(&cvars, "debug_log");
                config.screen_width  = cvar_get(&cvars, "screen.width");
                config.screen_height = cvar_get(&cvars, "screen.height");

                config.client_name     = cvar_get(&cvars, "client.name");
                config.client_email    = cvar_get(&cvars, "client.email");
                config.client_password = cvar_get(&cvars, "client.password");

                if (config.version) printf("version: %d\n", config.version->value.i);
                if (config.name) printf("name: %s\n", config.name->value.str.data);
                if (config.email) printf("email: %s\n", config.email->value.str.data);
                if (config.float_var)
                    printf("float_var: %f\n", config.float_var->value.f);
                if (config.password) {
                    printf("password: %s\n", config.password->value.str.data);
                }
                if (config.debug_log)
                    printf("debug_log: %b\n", config.debug_log->value.b);
                if (config.screen_width)
                    printf("screen_width: %d\n", config.screen_width->value.i);
                if (config.screen_height)
                    printf("screen_height: %d\n", config.screen_height->value.i);

                if (config.client_name)
                    printf("client_name: %s\n", config.client_name->value.str.data);
                if (config.client_email)
                    printf("client_email: %s\n", config.client_email->value.str.data);
                if (config.client_password)
                    printf("client_password: %s\n",
                           config.client_password->value.str.data);
            }
        }
        uint64_t current_time = SDL_GetTicksNS();
        frame_count++;

        if (current_time - last_fps_time >= 1000000000ULL) {
            float fps = (float)frame_count * 1000000000.0f /
                        (float)(current_time - last_fps_time);
            snprintf(fps_buffer, sizeof(fps_buffer), "FPS: %.0f", fps);
            frame_count   = 0;
            last_fps_time = current_time;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Update
        movingRect.x += 1;
        if (movingRect.x > 800) movingRect.x = -60;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderLine(renderer, 100, 100, 700, 500);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect staticRect = {200, 150, 400, 300};
        SDL_RenderFillRect(renderer, &staticRect);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer, &movingRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(renderer, 10, 10, fps_buffer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    cvar_destroy(&cvars);
    file_watcher_close(w);
    fclose(fp);
    return 0;
}
