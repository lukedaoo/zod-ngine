#ifndef RENDER_INTERNAL_H
#define RENDER_INTERNAL_H

#define RENDER_BACKEND_OPENGL 0
#define RENDER_BACKEND_VULKAN 1

#ifndef RENDER_BACKEND
#define RENDER_BACKEND RENDER_BACKEND_OPENGL
#endif

typedef struct SDL_Window SDL_Window;

typedef enum render_backend_type {
    BACKEND_TYPE_OPENGL = RENDER_BACKEND_OPENGL,
    BACKEND_TYPE_VULKAN = RENDER_BACKEND_VULKAN,
} render_backend_type;

typedef struct render_backend {
    render_backend_type type;
    void                *context;
} render_backend;

void *render_backend_context_create(SDL_Window *window);
void  render_backend_init(void *render_context, int width, int height);
void  render_backend_resize(void *render_context, int width, int height);
void  render_backend_begin(void *render_context);
void  render_backend_end(void *render_context);
void  render_backend_shutdown(void *render_context);

#endif
