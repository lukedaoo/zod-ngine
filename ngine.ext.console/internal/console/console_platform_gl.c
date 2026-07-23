#ifdef ZOD_NGINE_IMPLEMENTATION

#include "console_internal.h"

#if ZOD_CONSOLE_ENABLED

#include "ngine.core/internal/render/render_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL

#include <stddef.h>

#include <glad/gl.h>
#include <ngine.lib/log.h>
#include <ngine.lib/simple_font.h>

#include "ngine.core/render_text.h"
#include "ngine.core/zod_ngine.h"
#include "ngine.core/internal/engine_context/engine_context_internal.h"
#include "console_internal.h"

#ifndef CONSOLE_MAX_RECTS
#define CONSOLE_MAX_RECTS 8
#endif

#define CONSOLE_RECT_VERTS_PER_QUAD 6

typedef struct console_rect_vertex {
    float x, y;
    float r, g, b, a;
} console_rect_vertex;

static struct {
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLint  u_viewport;
    bool   ready;

    console_rect_vertex vertices[CONSOLE_MAX_RECTS * CONSOLE_RECT_VERTS_PER_QUAD];
    int                 quad_count;
} console_gl_state;

static const char *CONSOLE_QUAD_VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "layout(location=1) in vec4 color;\n"
     "uniform vec2 viewport;\n"
     "out vec4 v_color;\n"
     "void main() {\n"
     "    vec2 ndc = (pos / viewport) * 2.0 - 1.0;\n"
     "    ndc.y    = -ndc.y;\n"
     "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
     "    v_color = color;\n"
     "}\n";

static const char *CONSOLE_QUAD_FRAG_SRC =
     "#version 460 core\n"
     "in vec4 v_color;\n"
     "out vec4 frag_color;\n"
     "void main() {\n"
     "    frag_color = v_color;\n"
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

    glGenVertexArrays(1, &console_gl_state.vao);
    glGenBuffers(1, &console_gl_state.vbo);
    glBindVertexArray(console_gl_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, console_gl_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(console_gl_state.vertices), NULL,
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(console_rect_vertex),
                          (const void *)offsetof(console_rect_vertex, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(console_rect_vertex),
                          (const void *)offsetof(console_rect_vertex, r));
    glBindVertexArray(0);

    console_gl_state.quad_count = 0;
    console_gl_state.ready      = true;
    log_debug("console.init: gl backend ready");
}

static void console_queue_rect(float x, float y, float w, float h, color4f color) {
    if (console_gl_state.quad_count >= CONSOLE_MAX_RECTS) return;

    float x0 = x, y0 = y, x1 = x + w, y1 = y + h;

    console_rect_vertex *v =
         &console_gl_state.vertices[(ptrdiff_t)console_gl_state.quad_count *
                                    CONSOLE_RECT_VERTS_PER_QUAD];

    // clang-format off
    v[0] = (console_rect_vertex){.x = x0, .y = y0, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    v[1] = (console_rect_vertex){.x = x1, .y = y0, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    v[2] = (console_rect_vertex){.x = x1, .y = y1, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    v[3] = (console_rect_vertex){.x = x0, .y = y0, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    v[4] = (console_rect_vertex){.x = x1, .y = y1, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    v[5] = (console_rect_vertex){.x = x0, .y = y1, .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    // clang-format on

    console_gl_state.quad_count++;
}

static void console_flush_rects(void) {
    if (console_gl_state.quad_count == 0) return;

    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(console_gl_state.shader);
    glUniform2f(console_gl_state.u_viewport, vw, vh);

    glBindVertexArray(console_gl_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, console_gl_state.vbo);
    glBufferSubData(
         GL_ARRAY_BUFFER, 0,
         (GLsizeiptr)((size_t)console_gl_state.quad_count * CONSOLE_RECT_VERTS_PER_QUAD *
                      sizeof(console_rect_vertex)),
         console_gl_state.vertices);

    glDrawArrays(GL_TRIANGLES, 0,
                 console_gl_state.quad_count * CONSOLE_RECT_VERTS_PER_QUAD);

    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);

    console_gl_state.quad_count = 0;
}

static void console_platform_draw_panel(int width, int height) {
    console_queue_rect(0.0f, 0.0f, (float)width, (float)height,
                       g_console.background_color);
}

static void console_platform_draw_input_box(float x, float y, float w, float h) {
    const color4f border = g_console.input_box_color;
    float         s      = g_console.input_box_stroke;

    console_queue_rect(x, y, w, h, g_console.input_box_background_color);
    console_queue_rect(x, y, w, s, border);
    console_queue_rect(x, y + h - s, w, s, border);
    console_queue_rect(x, y, s, h, border);
    console_queue_rect(x + w - s, y, s, h, border);
}

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

void console_priv_platform_draw(int width, int height, int lines_fit) {
    if (!console_gl_state.ready) console_platform_init();

    const simple_font *font  = zngine_font_primary_get();
    float              scale = g_console.font_size / (float)simple_font_get_advance(font);
    float              row_height = g_console.font_size * CONSOLE_LINE_HEIGHT_RATIO;
    float              line_offset =
         console_line_offset(font, scale, g_console.font_size, row_height) +
         g_console.top_pad;

    int scrollback_rows = lines_fit > 0 ? lines_fit - 1 : 0;

    console_platform_draw_panel(width, height);

    float row_top = 0.0f, box_x = 0.0f, box_w = 0.0f;
    if (lines_fit > 0) {
        row_top = (float)scrollback_rows * row_height + g_console.top_pad +
                  g_console.input_gap;
        box_x   = g_console.input_box_margin;
        box_w   = (float)width - 2.0f * g_console.input_box_margin;
        console_platform_draw_input_box(box_x, row_top, box_w, row_height);
    }

    float scrollback_clip_bottom = lines_fit > 0 ? row_top : (float)height;
    int   start = console_priv_visible_line_start(g_console.count, scrollback_rows,
                                                  g_console.scroll_offset);
    for (int i = start; i < g_console.count; i++) {
        render_text_draw_clipped(
             g_console.text_pad_x, (float)(i - start) * row_height + line_offset,
             g_console.lines[i], scale, g_console.lines_color[i], font, 0.05f, 0.0f, 0.0f,
             (float)width, scrollback_clip_bottom);
    }

    console_flush_rects();
    render_text_flush();

    if (lines_fit > 0) {
        float input_y =
             (float)scrollback_rows * row_height + line_offset + g_console.input_gap;
        float available_width = box_w - g_console.text_pad_x - g_console.input_right_pad;
        int   scroll_start    = console_input_scroll_start(
             g_console.input, g_console.cursor_pos, available_width, font, scale);

        // Clipped to the box's own interior — scroll_start only guarantees the
        // cursor fits, text past the cursor (or an unscrolled string longer than
        // the box) would otherwise still extend past the box edges.
        float stroke      = g_console.input_box_stroke;
        float clip_left   = box_x + stroke;
        float clip_top    = row_top + stroke;
        float clip_right  = box_x + box_w - stroke;
        float clip_bottom = row_top + row_height - stroke;

        render_text_draw_clipped(box_x + g_console.text_pad_x, input_y,
                                 g_console.input + scroll_start, scale,
                                 g_console.input_text_color, font, 0.0f, clip_left,
                                 clip_top, clip_right, clip_bottom);
        float cursor_x =
             box_x + g_console.text_pad_x +
             console_measure_text_width(g_console.input + scroll_start,
                                        g_console.cursor_pos - scroll_start, font, scale);
        render_text_flush();

        if (g_console.cursor_pos > scroll_start) {
            const simple_font_glyph *last_glyph =
                 simple_font_get_glyph(font, g_console.input[g_console.cursor_pos - 1]);
            float overhang =
                 last_glyph && last_glyph->width > last_glyph->advance
                      ? (float)(last_glyph->width - last_glyph->advance) * scale * 0.5f
                      : 0.0f;
            cursor_x += fmaxf(1.0f, overhang + 1.0f);
        } else {
            cursor_x += 1.0f;
        }
        float caret_w = g_console.font_size * 0.4f;

        caret_w = fminf(caret_w, clip_right - cursor_x);
        if (caret_w > 0.0f) {
            float caret_height = g_console.font_size * 0.7f;
            float caret_top    = row_top + (row_height - caret_height) * 0.5f;
            console_queue_rect(cursor_x, caret_top, caret_w, caret_height,
                               g_console.cursor_color);
        }
        console_flush_rects();
    }
}

#endif

#else

void console_priv_platform_draw(int width, int height, int lines_fit) {
    (void)width;
    (void)height;
    (void)lines_fit;
}

#endif

#endif  // ZOD_NGINE_IMPLEMENTATION
