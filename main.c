#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <stdlib.h>
#include <stdio.h>
#define INI_IMPLEMENTATION
#include "modules/ini.h"

struct Window {
    uint32_t      width;
    uint32_t      height;
    SDL_Window   *window;
    SDL_Renderer *renderer;
};

typedef struct {
    int         version;
    const char *name;
    const char *email;
} configuration;

static int handler(const char *section,
                   const char *name,
                   const char *value,
                   void       *user) {
    configuration *pconfig = (configuration *)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("protocol", "version")) {
        pconfig->version = atoi(value);
    } else if (MATCH("user", "name")) {
        pconfig->name = strdup(value);
    } else if (MATCH("user", "email")) {
        pconfig->email = strdup(value);
    } else {
        return 0;
    }
    return 1;
}

int main(void) {
    configuration config;

    if (ini_parse("test.ini", handler, &config) < 0) {
        printf("Can't load 'test.ini'\n");
        return 1;
    }

    printf("version: %d\n", config.version);
    printf("name: %s\n", config.name);
    printf("email: %s\n", config.email);

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

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
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
    while (running) {
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
    return 0;
}
