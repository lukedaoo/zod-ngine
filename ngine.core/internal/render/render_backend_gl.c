#ifdef ZOD_NGINE_IMPLEMENTATION

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <ngine.lib/log.h>

#include "render_internal.h"

typedef struct gl_backend_context {
    SDL_Window   *window;
    SDL_GLContext gl;
} gl_backend_context;

static gl_backend_context gl_ctx;

void *render_backend_context_create(SDL_Window *window) {
    gl_ctx.window = window;
    gl_ctx.gl     = NULL;
    return &gl_ctx;
}

void render_backend_init(void *render_context, int width, int height) {
    gl_backend_context *c = render_context;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    // alpha-capable framebuffer required for compositor to honor window transparency
    if (SDL_GetWindowFlags(c->window) & SDL_WINDOW_TRANSPARENT) {
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    }
    c->gl = SDL_GL_CreateContext(c->window);
    if (!c->gl) {
        log_fatal("render.gl_init: SDL_GL_CreateContext failed — %s", SDL_GetError());
    }
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        log_fatal("render.gl_init: gladLoadGL failed");
    }
    glViewport(0, 0, width, height);
}

void render_backend_resize(void *render_context, int width, int height) {
    (void)render_context;
    glViewport(0, 0, width, height);
}

void render_backend_begin(void *render_context) {
    (void)render_context;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void render_backend_end(void *render_context) {
    gl_backend_context *c = render_context;
    SDL_GL_SwapWindow(c->window);
}

void render_backend_shutdown(void *render_context) {
    gl_backend_context *c = render_context;
    if (c->gl) SDL_GL_DestroyContext(c->gl);
    c->gl = NULL;
}

#endif
