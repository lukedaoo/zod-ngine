#ifdef ZOD_NGINE_IMPLEMENTATION

#include "render_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL

#include <string.h>

#include <glad/gl.h>
#include <modules/log.h>
#include <modules/simple_font.h>

#include "../../render_text.h"
#include "../engine_context/engine_context_internal.h"

static struct {
    GLuint      shader;
    GLuint      vao;
    GLuint      vbo;
    GLuint      atlas_texture;
    GLint       u_rect;
    GLint       u_viewport;
    GLint       u_uv_rect;
    GLint       u_color;
    simple_font font;
} render_text_state;

static const char *RENDER_TEXT_VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "uniform vec4 rect;\n"
     "uniform vec2 viewport;\n"
     "uniform vec4 uv_rect;\n"
     "out vec2 v_uv;\n"
     "void main() {\n"
     "    vec2 p   = rect.xy + pos * rect.zw;\n"
     "    vec2 ndc = (p / viewport) * 2.0 - 1.0;\n"
     "    ndc.y    = -ndc.y;\n"
     "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
     "    v_uv = uv_rect.xy + pos * uv_rect.zw;\n"
     "}\n";

static const char *RENDER_TEXT_FRAG_SRC =
     "#version 460 core\n"
     "in vec2 v_uv;\n"
     "uniform sampler2D atlas;\n"
     "uniform vec4 color;\n"
     "out vec4 frag_color;\n"
     "void main() {\n"
     "    float a = texture(atlas, v_uv).r;\n"
     "    frag_color = vec4(color.rgb, color.a * a);\n"
     "}\n";

static GLuint render_text_compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), NULL, buf);
        log_error("render_text.shader: compile failed — %s", buf);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

void render_text_init(const simple_font *font) {
    if (!font) {
        log_error("render_text: font is null");
        return;
    }
    GLuint vert = render_text_compile_shader(GL_VERTEX_SHADER, RENDER_TEXT_VERT_SRC);
    GLuint frag = render_text_compile_shader(GL_FRAGMENT_SHADER, RENDER_TEXT_FRAG_SRC);

    render_text_state.shader = glCreateProgram();
    glAttachShader(render_text_state.shader, vert);
    glAttachShader(render_text_state.shader, frag);
    glLinkProgram(render_text_state.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    render_text_state.u_rect = glGetUniformLocation(render_text_state.shader, "rect");
    render_text_state.u_viewport =
         glGetUniformLocation(render_text_state.shader, "viewport");
    render_text_state.u_uv_rect =
         glGetUniformLocation(render_text_state.shader, "uv_rect");
    render_text_state.u_color = glGetUniformLocation(render_text_state.shader, "color");

    // clang-format off
    static const float verts[] = {
         0.0f, 0.0f,
         1.0f, 0.0f,
         1.0f, 1.0f,

         0.0f, 0.0f,
         1.0f, 1.0f,
         0.0f, 1.0f,
    };
    // clang-format on
    glGenVertexArrays(1, &render_text_state.vao);
    glGenBuffers(1, &render_text_state.vbo);
    glBindVertexArray(render_text_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_text_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glBindVertexArray(0);

    // @todo:
    // have global fonts
    render_text_state.font  = *font;
    simple_font_atlas atlas = simple_font_get_atlas(&render_text_state.font);

    glGenTextures(1, &render_text_state.atlas_texture);
    glBindTexture(GL_TEXTURE_2D, render_text_state.atlas_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas.width, atlas.height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, atlas.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    log_debug("render_text.init: ready");
}

void render_text_destroy(void) {
    glDeleteProgram(render_text_state.shader);
    glDeleteVertexArrays(1, &render_text_state.vao);
    glDeleteBuffers(1, &render_text_state.vbo);
    glDeleteTextures(1, &render_text_state.atlas_texture);
}

void render_text_draw(float x, float y, const char *str, float scale, color4f color) {
    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    simple_font_atlas atlas            = simple_font_get_atlas(&render_text_state.font);
    int               fallback_advance = simple_font_get_advance(&render_text_state.font);

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(render_text_state.shader);
    glUniform2f(render_text_state.u_viewport, vw, vh);
    glUniform4f(render_text_state.u_color, color.r, color.g, color.b, color.a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_text_state.atlas_texture);
    glBindVertexArray(render_text_state.vao);

    float cursor_x = x;
    for (const char *p = str; *p; p++) {
        const simple_font_glyph *g = simple_font_get_glyph(&render_text_state.font, *p);
        if (g) {
            glUniform4f(render_text_state.u_rect, cursor_x,
                        y + (float)g->y_offset * scale, (float)g->width * scale,
                        (float)g->height * scale);
            glUniform4f(render_text_state.u_uv_rect, (float)g->x / (float)atlas.width,
                        (float)g->y / (float)atlas.height,
                        (float)g->width / (float)atlas.width,
                        (float)g->height / (float)atlas.height);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            cursor_x += (float)g->advance * scale;
        } else {
            cursor_x += (float)fallback_advance * scale;
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);
}

#else  // RENDER_BACKEND != RENDER_BACKEND_OPENGL — render_text not ported to this backend
       // yet

#include "../../render_text.h"

void render_text_init(const simple_font *font) { (void)font; }
void render_text_destroy(void) {}
void render_text_draw(float x, float y, const char *str, float scale, color4f color) {
    (void)x;
    (void)y;
    (void)str;
    (void)scale;
    (void)color;
}

#endif

#endif
