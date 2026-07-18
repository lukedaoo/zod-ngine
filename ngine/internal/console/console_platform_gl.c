#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../render/render_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL

#include <glad/gl.h>
#include <modules/log.h>

#include "../../render_text.h"
#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"
#include "console_internal.h"

typedef struct console_quad_vertex {
    float x, y;
} console_quad_vertex;

static struct {
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLint  u_viewport;
    GLint  u_size;
    GLint  u_color;
    bool   ready;
} console_gl_state;

static const char *CONSOLE_QUAD_VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "uniform vec2 viewport;\n"
     "uniform vec2 size;\n"
     "void main() {\n"
     "    vec2 ndc = ((pos * size) / viewport) * 2.0 - 1.0;\n"
     "    ndc.y    = -ndc.y;\n"
     "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
     "}\n";

static const char *CONSOLE_QUAD_FRAG_SRC =
     "#version 460 core\n"
     "uniform vec4 color;\n"
     "out vec4 frag_color;\n"
     "void main() {\n"
     "    frag_color = color;\n"
     "}\n";

static GLuint console_compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), NULL, buf);
        log_error("console.shader: compile failed — %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static void console_platform_init(void) {
    GLuint vert = console_compile_shader(GL_VERTEX_SHADER, CONSOLE_QUAD_VERT_SRC);
    GLuint frag = console_compile_shader(GL_FRAGMENT_SHADER, CONSOLE_QUAD_FRAG_SRC);

    console_gl_state.shader = glCreateProgram();
    glAttachShader(console_gl_state.shader, vert);
    glAttachShader(console_gl_state.shader, frag);
    glLinkProgram(console_gl_state.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    console_gl_state.u_viewport =
         glGetUniformLocation(console_gl_state.shader, "viewport");
    console_gl_state.u_size  = glGetUniformLocation(console_gl_state.shader, "size");
    console_gl_state.u_color = glGetUniformLocation(console_gl_state.shader, "color");

    // clang-format off
    const console_quad_vertex verts[6] = {
         {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
         {0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},
    };
    // clang-format on

    glGenVertexArrays(1, &console_gl_state.vao);
    glGenBuffers(1, &console_gl_state.vbo);
    glBindVertexArray(console_gl_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, console_gl_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(console_quad_vertex), NULL);
    glBindVertexArray(0);

    console_gl_state.ready = true;
    log_debug("console.init: gl backend ready");
}

static void console_platform_draw_panel(int width, int height) {
    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(console_gl_state.shader);
    glUniform2f(console_gl_state.u_viewport, vw, vh);
    glUniform2f(console_gl_state.u_size, (float)width, (float)height);
    glUniform4f(console_gl_state.u_color, 0.0f, 0.0f, 0.0f, 0.85f);

    glBindVertexArray(console_gl_state.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);
}

void console_platform_draw(int width, int height) {
    if (!console_gl_state.ready) console_platform_init();

    console_platform_draw_panel(width, height);

    const color4f      text_color = {1.0f, 1.0f, 1.0f, 1.0f};
    const simple_font *font       = zod_font_primary_get();
    int                lines_fit  = height / CONSOLE_LINE_HEIGHT;
    int                start = console_visible_line_start(g_console.count, lines_fit);
    for (int i = start; i < g_console.count; i++) {
        render_text_draw(4.0f, (float)((i - start) * CONSOLE_LINE_HEIGHT) + 16.0f,
                         g_console.lines[i], 1.0f, text_color, font);
    }
    render_text_flush();
}

#endif

#endif
