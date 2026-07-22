#ifdef ZOD_NGINE_IMPLEMENTATION

#include <SDL3/SDL.h>

#include <ngine.lib/log.h>

#include "render_internal.h"

typedef struct vulkan_backend_context {
    SDL_Window *window;
} vulkan_backend_context;

static vulkan_backend_context vulkan_ctx;

void *render_backend_priv_context_create(SDL_Window *window) {
    vulkan_ctx.window = window;
    return &vulkan_ctx;
}

void render_backend_priv_init(void *render_context, int width, int height) {
    (void)render_context;
    (void)width;
    (void)height;
    log_fatal("render.vk_init: vulkan backend not implemented");
}

void render_backend_priv_resize(void *render_context, int width, int height) {
    (void)render_context;
    (void)width;
    (void)height;
    log_fatal("render.vk_resize: vulkan backend not implemented");
}

void render_backend_priv_begin(void *render_context) {
    (void)render_context;
    log_fatal("render.vk_begin: vulkan backend not implemented");
}

void render_backend_priv_end(void *render_context) {
    (void)render_context;
    log_fatal("render.vk_end: vulkan backend not implemented");
}

void render_backend_priv_shutdown(void *render_context) {
    (void)render_context;
    log_fatal("render.vk_shutdown: vulkan backend not implemented");
}

#endif
