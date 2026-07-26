#ifdef ZOD_NGINE_IMPLEMENTATION

#include "render_internal.h"

#include "../../render_sprite.h"
#include "../engine_context/engine_context_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL

#include <stddef.h>

#include <glad/gl.h>
#include <ngine.lib/log.h>

#define STB_IMAGE_IMPLEMENTATION
#include <thirdparty/stb_image.h>

#ifndef RENDER_SPRITE_MAX_QUADS
#define RENDER_SPRITE_MAX_QUADS 256
#endif

#define RENDER_SPRITE_VERTS_PER_QUAD 6

typedef struct render_sprite_vertex {
    float x, y;
    float u, v;
    float r, g, b, a;
} render_sprite_vertex;

static struct {
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLint  u_viewport;

    GLuint active_texture;  // texture bound for the pending batch; 0 = none queued

    render_sprite_vertex vertices[RENDER_SPRITE_MAX_QUADS * RENDER_SPRITE_VERTS_PER_QUAD];
    int                  quad_count;
} render_sprite_state;

static const char *RENDER_SPRITE_VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "layout(location=1) in vec2 uv;\n"
     "layout(location=2) in vec4 color;\n"
     "uniform vec2 viewport;\n"
     "out vec2 v_uv;\n"
     "out vec4 v_color;\n"
     "void main() {\n"
     "    vec2 ndc = (pos / viewport) * 2.0 - 1.0;\n"
     "    ndc.y    = -ndc.y;\n"
     "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
     "    v_uv     = uv;\n"
     "    v_color  = color;\n"
     "}\n";

static const char *RENDER_SPRITE_FRAG_SRC =
     "#version 460 core\n"
     "in vec2 v_uv;\n"
     "in vec4 v_color;\n"
     "uniform sampler2D tex;\n"
     "out vec4 frag_color;\n"
     "void main() {\n"
     "    frag_color = texture(tex, v_uv) * v_color;\n"
     "}\n";

static GLuint render_sprite_compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), NULL, buf);
        log_error("render_sprite.shader: compile failed — %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

void render_sprite_init(void) {
    GLuint vert = render_sprite_compile_shader(GL_VERTEX_SHADER, RENDER_SPRITE_VERT_SRC);
    GLuint frag =
         render_sprite_compile_shader(GL_FRAGMENT_SHADER, RENDER_SPRITE_FRAG_SRC);

    render_sprite_state.shader = glCreateProgram();
    glAttachShader(render_sprite_state.shader, vert);
    glAttachShader(render_sprite_state.shader, frag);
    glLinkProgram(render_sprite_state.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    render_sprite_state.u_viewport =
         glGetUniformLocation(render_sprite_state.shader, "viewport");

    glGenVertexArrays(1, &render_sprite_state.vao);
    glGenBuffers(1, &render_sprite_state.vbo);
    glBindVertexArray(render_sprite_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_sprite_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(render_sprite_state.vertices), NULL,
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(render_sprite_vertex),
                          (const void *)offsetof(
                               render_sprite_vertex,  // NOLINT(performance-no-int-to-ptr)
                               x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(render_sprite_vertex),
                          (const void *)offsetof(
                               render_sprite_vertex,  // NOLINT(performance-no-int-to-ptr)
                               u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(render_sprite_vertex),
                          (const void *)offsetof(
                               render_sprite_vertex,  // NOLINT(performance-no-int-to-ptr)
                               r));
    glBindVertexArray(0);

    render_sprite_state.quad_count     = 0;
    render_sprite_state.active_texture = 0;

    log_debug("render_sprite.init: ready");
}

void render_sprite_destroy(void) {
    glDeleteProgram(render_sprite_state.shader);
    glDeleteVertexArrays(1, &render_sprite_state.vao);
    glDeleteBuffers(1, &render_sprite_state.vbo);
}

sprite_texture sprite_texture_load(const char *path) {
    int            w = 0, h = 0, channels = 0;
    unsigned char *pixels = stbi_load(path, &w, &h, &channels, 4);
    if (!pixels) {
        log_error("render_sprite.texture_load: failed to load '%s': %s", path,
                  stbi_failure_reason());
        return (sprite_texture){0};
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    log_debug("render_sprite.texture_load: loaded '%s' (%dx%d)", path, w, h);
    return (sprite_texture){.id = id, .width = w, .height = h};
}

sprite_texture sprite_texture_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    unsigned char pixel[4] = {r, g, b, a};

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return (sprite_texture){.id = id, .width = 1, .height = 1};
}

void sprite_texture_unload(sprite_texture *tex) {
    if (tex->id != 0) {
        GLuint id = (GLuint)tex->id;
        glDeleteTextures(1, &id);
    }
    *tex = (sprite_texture){0};
}

void render_sprite_draw(sprite_texture tex, float x, float y, float w, float h,
                        color4f tint) {
    if (tex.id == 0) return;

    if (render_sprite_state.quad_count > 0 &&
        render_sprite_state.active_texture != (GLuint)tex.id) {
        render_sprite_flush();
    }
    render_sprite_state.active_texture = (GLuint)tex.id;

    if (render_sprite_state.quad_count >= RENDER_SPRITE_MAX_QUADS) return;

    float x0 = x, y0 = y, x1 = x + w, y1 = y + h;

    render_sprite_vertex *v =
         &render_sprite_state.vertices[(ptrdiff_t)render_sprite_state.quad_count *
                                       RENDER_SPRITE_VERTS_PER_QUAD];

    // clang-format off
    v[0] = (render_sprite_vertex){x0, y0, 0.0f, 0.0f, tint.r, tint.g, tint.b, tint.a};
    v[1] = (render_sprite_vertex){x1, y0, 1.0f, 0.0f, tint.r, tint.g, tint.b, tint.a};
    v[2] = (render_sprite_vertex){x1, y1, 1.0f, 1.0f, tint.r, tint.g, tint.b, tint.a};
    v[3] = (render_sprite_vertex){x0, y0, 0.0f, 0.0f, tint.r, tint.g, tint.b, tint.a};
    v[4] = (render_sprite_vertex){x1, y1, 1.0f, 1.0f, tint.r, tint.g, tint.b, tint.a};
    v[5] = (render_sprite_vertex){x0, y1, 0.0f, 1.0f, tint.r, tint.g, tint.b, tint.a};
    // clang-format on

    render_sprite_state.quad_count++;
}

void render_sprite_flush(void) {
    if (render_sprite_state.quad_count == 0) return;

    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(render_sprite_state.shader);
    glUniform2f(render_sprite_state.u_viewport, vw, vh);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_sprite_state.active_texture);
    glBindVertexArray(render_sprite_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_sprite_state.vbo);
    glBufferSubData(
         GL_ARRAY_BUFFER, 0,
         (GLsizeiptr)((size_t)render_sprite_state.quad_count *
                      RENDER_SPRITE_VERTS_PER_QUAD * sizeof(render_sprite_vertex)),
         render_sprite_state.vertices);

    glDrawArrays(GL_TRIANGLES, 0,
                 render_sprite_state.quad_count * RENDER_SPRITE_VERTS_PER_QUAD);

    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);

    render_sprite_state.quad_count     = 0;
    render_sprite_state.active_texture = 0;
}

#else  // RENDER_BACKEND != RENDER_BACKEND_OPENGL — render_sprite not ported to this
       // backend yet

sprite_texture sprite_texture_load(const char *path) {
    (void)path;
    return (sprite_texture){0};
}
sprite_texture sprite_texture_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)r;
    (void)g;
    (void)b;
    (void)a;
    return (sprite_texture){0};
}
void sprite_texture_unload(sprite_texture *tex) { *tex = (sprite_texture){0}; }
void render_sprite_init(void) {}
void render_sprite_destroy(void) {}
void render_sprite_draw(sprite_texture tex, float x, float y, float w, float h,
                        color4f tint) {
    (void)tex;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)tint;
}
void render_sprite_flush(void) {}

#endif

#endif  // ZOD_NGINE_IMPLEMENTATION
