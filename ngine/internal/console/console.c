#ifdef ZOD_NGINE_IMPLEMENTATION

#include <glad/gl.h>
#include <modules/log.h>

#include "../../console.h"
#include "../engine_context/engine_context_internal.h"

static struct {
    bool   open;
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLint  u_rect;
    GLint  u_viewport;
    GLint  u_color;
} console_state;

static const char *VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "uniform vec4 rect;\n"
     "uniform vec2 viewport;\n"
     "void main() {\n"
     "    vec2 p   = rect.xy + pos * rect.zw;\n"
     "    vec2 ndc = (p / viewport) * 2.0 - 1.0;\n"
     "    ndc.y    = -ndc.y;\n"
     "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
     "}\n";

static const char *FRAG_SRC =
     "#version 460 core\n"
     "uniform vec4 color;\n"
     "out vec4 frag_color;\n"
     "void main() { frag_color = color; }\n";

static GLuint compile_shader(GLenum type, const char *src) {
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

void zod_console_init(void) {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);

    console_state.shader = glCreateProgram();
    glAttachShader(console_state.shader, vert);
    glAttachShader(console_state.shader, frag);
    glLinkProgram(console_state.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    console_state.u_rect     = glGetUniformLocation(console_state.shader, "rect");
    console_state.u_viewport = glGetUniformLocation(console_state.shader, "viewport");
    console_state.u_color    = glGetUniformLocation(console_state.shader, "color");

    // clang-format off
    static const float verts[] = {
         0.0f, 0.0f, 1.0f,
         0.0f, 1.0f, 1.0f,
         0.0f, 0.0f, 1.0f,
         1.0f, 0.0f, 1.0f,
    };
    // clang-format on
    glGenVertexArrays(1, &console_state.vao);
    glGenBuffers(1, &console_state.vbo);
    glBindVertexArray(console_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, console_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glBindVertexArray(0);

    log_debug("console.init: ready");
}

void zod_console_destroy(void) {
    glDeleteProgram(console_state.shader);
    glDeleteVertexArrays(1, &console_state.vao);
    glDeleteBuffers(1, &console_state.vbo);
}

void zod_console_toggle(void) { console_state.open = !console_state.open; }
bool zod_console_is_open(void) { return console_state.open; }

static void draw_rect(float x, float y, float w, float h, float r, float g, float b,
                      float a) {
    glUniform4f(console_state.u_rect, x, y, w, h);
    glUniform4f(console_state.u_color, r, g, b, a);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void zod_console_draw(void) {
    if (!console_state.open) return;

    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    float panel_h = vh * 0.40f;
    float input_h = 28.0f;
    float pad     = 8.0f;

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(console_state.shader);
    glUniform2f(console_state.u_viewport, vw, vh);
    glBindVertexArray(console_state.vao);

    draw_rect(0.0f, 0.0f, vw, panel_h, 0.08f, 0.08f, 0.10f, 0.92f);

    draw_rect(0.0f, panel_h - 1.0f, vw, 1.0f, 0.28f, 0.50f, 0.62f, 1.0f);

    draw_rect(pad, panel_h - input_h - pad, vw - pad * 2.0f, input_h, 0.14f, 0.14f, 0.18f,
              1.0f);

    draw_rect(pad, panel_h - input_h - pad, vw - pad * 2.0f, 1.0f, 0.28f, 0.50f, 0.62f,
              0.7f);

    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);
}

#endif
