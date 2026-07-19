#ifdef ZOD_NGINE_IMPLEMENTATION

#include "console_internal.h"

#if ZOD_CONSOLE_ENABLED

#include "ngine.core/internal/render/render_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL

#include <glad/gl.h>
#include <ngine.lib/log.h>
#include <ngine.lib/simple_font.h>

#include "ngine.core/render_text.h"
#include "ngine.core/zod_ngine.h"
#include "ngine.core/internal/engine_context/engine_context_internal.h"
#include "console_internal.h"

static struct {
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLint  u_viewport;
    GLint  u_offset;
    GLint  u_size;
    GLint  u_color;
    bool   ready;
} console_gl_state;

static const char *CONSOLE_QUAD_VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "uniform vec2 viewport;\n"
     "uniform vec2 offset;\n"
     "uniform vec2 size;\n"
     "void main() {\n"
     "    vec2 ndc = (((pos * size) + offset) / viewport) * 2.0 - 1.0;\n"
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
    console_gl_state.u_offset = glGetUniformLocation(console_gl_state.shader, "offset");
    console_gl_state.u_size   = glGetUniformLocation(console_gl_state.shader, "size");
    console_gl_state.u_color  = glGetUniformLocation(console_gl_state.shader, "color");

    // clang-format off
    const float verts[12] = {
         0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
         0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
    };
    // clang-format on

    glGenVertexArrays(1, &console_gl_state.vao);
    glGenBuffers(1, &console_gl_state.vbo);
    glBindVertexArray(console_gl_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, console_gl_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL);
    glBindVertexArray(0);

    console_gl_state.ready = true;
    log_debug("console.init: gl backend ready");
}

static void console_draw_rect(float x, float y, float w, float h, color4f color) {
    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(console_gl_state.shader);
    glUniform2f(console_gl_state.u_viewport, vw, vh);
    glUniform2f(console_gl_state.u_offset, x, y);
    glUniform2f(console_gl_state.u_size, w, h);
    glUniform4f(console_gl_state.u_color, color.r, color.g, color.b, color.a);

    glBindVertexArray(console_gl_state.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);
}

static void console_platform_draw_panel(int width, int height) {
    console_draw_rect(0.0f, 0.0f, (float)width, (float)height,
                      g_console.background_color);
}

static void console_platform_draw_input_box(float x, float y, float w, float h) {
    const color4f border = g_console.input_box_color;
    float         s      = g_console.input_box_stroke;

    console_draw_rect(x, y, w, h, g_console.input_box_background_color);
    console_draw_rect(x, y, w, s, border);
    console_draw_rect(x, y + h - s, w, s, border);
    console_draw_rect(x, y, s, h, border);
    console_draw_rect(x + w - s, y, s, h, border);
}

// render_text's y param is the visual top for the ascii backend (glyph
// y_offset is always 0) but the *baseline* for ttf (y_offset is
// baseline-relative) — so centering the glyph's rendered height (== font_size)
// within a row of row_height needs converting "desired visual top" into
// whatever y actually means for this backend, via simple_font_get_baseline.
static float console_line_offset(const simple_font *font, float scale, float font_size,
                                 float row_height) {
    float visual_top = (row_height - font_size) * 0.5f;
    return visual_top + (float)simple_font_get_baseline(font) * scale;
}

static float console_measure_text_width(const char *str, int n, const simple_font *font,
                                        float scale) {
    int fallback_advance = simple_font_get_advance(font);
    int width            = 0;
    for (int i = 0; i < n; i++) {
        const simple_font_glyph *g = simple_font_get_glyph(font, str[i]);
        width += g ? g->advance : fallback_advance;
    }
    return (float)width * scale;
}

static int console_input_scroll_start(const char *input, int cursor_pos,
                                      float available_width, const simple_font *font,
                                      float scale) {
    int   fallback_advance = simple_font_get_advance(font);
    float used             = 0.0f;
    int   start            = cursor_pos;
    while (start > 0) {
        const simple_font_glyph *g   = simple_font_get_glyph(font, input[start - 1]);
        float                    adv = (float)(g ? g->advance : fallback_advance) * scale;
        if (used + adv > available_width) break;
        used += adv;
        start--;
    }
    return start;
}

void console_platform_draw(int width, int height) {
    if (!console_gl_state.ready) console_platform_init();

    console_platform_draw_panel(width, height);

    const simple_font *font  = zod_font_primary_get();
    float              scale = g_console.font_size / (float)simple_font_get_advance(font);
    float              row_height = g_console.font_size * CONSOLE_LINE_HEIGHT_RATIO;
    float              line_offset =
         console_line_offset(font, scale, g_console.font_size, row_height) +
         g_console.top_pad;

    int lines_fit =
         (int)(((float)height - g_console.top_pad - g_console.input_gap) / row_height);
    int scrollback_rows = lines_fit > 0 ? lines_fit - 1 : 0;
    int start           = console_visible_line_start(g_console.count, scrollback_rows);
    for (int i = start; i < g_console.count; i++) {
        render_text_draw_padded(0.0f, (float)(i - start) * row_height + line_offset,
                                g_console.lines[i], scale, g_console.output_text_color,
                                font, g_console.text_pad_x, 0.0f);
    }
    render_text_flush();

    if (lines_fit > 0) {
        float row_top = (float)scrollback_rows * row_height + g_console.top_pad +
                        g_console.input_gap;
        float box_x   = g_console.input_box_margin;
        float box_w   = (float)width - 2.0f * g_console.input_box_margin;
        console_platform_draw_input_box(box_x, row_top, box_w, row_height);

        float input_y =
             (float)scrollback_rows * row_height + line_offset + g_console.input_gap;
        // Available width is the box's own interior (box_w), not the panel's
        // full width — text must stay within the box it's visually drawn in.
        float available_width = box_w - g_console.text_pad_x - g_console.input_right_pad;
        int   scroll_start    = console_input_scroll_start(
             g_console.input, g_console.cursor_pos, available_width, font, scale);

        render_text_draw_padded(box_x, input_y, g_console.input + scroll_start, scale,
                                g_console.input_text_color, font, g_console.text_pad_x,
                                0.0f);
        float cursor_x =
             box_x + g_console.text_pad_x +
             console_measure_text_width(g_console.input + scroll_start,
                                        g_console.cursor_pos - scroll_start, font, scale);
        render_text_draw_basic(cursor_x, input_y, "|", scale, g_console.input_text_color,
                               font);

        // scroll_start only guarantees the cursor fits — text past the
        // cursor (or an unscrolled string longer than the box) still queues
        // quads beyond the box edges, so clip the flush to the box interior
        // instead of trying to bound it purely by character counting.
        float     stroke       = g_console.input_box_stroke;
        GLboolean prev_scissor = glIsEnabled(GL_SCISSOR_TEST);
        glEnable(GL_SCISSOR_TEST);
        glScissor((GLint)(box_x + stroke),
                  (GLint)((float)g_ctx.window.height - (row_top + row_height - stroke)),
                  (GLsizei)(box_w - 2.0f * stroke),
                  (GLsizei)(row_height - 2.0f * stroke));
        render_text_flush();
        if (!prev_scissor) glDisable(GL_SCISSOR_TEST);
    }
}

#endif  

#else  

void console_platform_draw(int width, int height) {
    (void)width;
    (void)height;
}

#endif  

#endif  // ZOD_NGINE_IMPLEMENTATION
