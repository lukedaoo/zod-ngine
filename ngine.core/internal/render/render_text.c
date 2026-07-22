#ifdef ZOD_NGINE_IMPLEMENTATION

#include "render_internal.h"

#include <math.h>

#include <ngine.lib/simple_font.h>

#include "../../render_text.h"
#include "../engine_context/engine_context_internal.h"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL

#include <stddef.h>

#include <glad/gl.h>
#include <ngine.lib/log.h>

#ifndef RENDER_TEXT_MAX_QUADS
#define RENDER_TEXT_MAX_QUADS 4096
#endif

#define RENDER_TEXT_VERTS_PER_QUAD 6

typedef struct render_text_vertex {
    float x, y;
    float u, v;
    float r, g, b, a;
    float weight;
} render_text_vertex;

static struct {
    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLuint atlas_texture;
    GLint  u_viewport;

    const simple_font *bound_font;  // identity tag only — not owned, not a copy
    bool               dirty;

    render_text_vertex vertices[RENDER_TEXT_MAX_QUADS * RENDER_TEXT_VERTS_PER_QUAD];
    int                quad_count;
} render_text_state;

static const char *RENDER_TEXT_VERT_SRC =
     "#version 460 core\n"
     "layout(location=0) in vec2 pos;\n"
     "layout(location=1) in vec2 uv;\n"
     "layout(location=2) in vec4 color;\n"
     "layout(location=3) in float weight;\n"
     "uniform vec2 viewport;\n"
     "out vec2 v_uv;\n"
     "out vec4 v_color;\n"
     "out float v_weight;\n"
     "void main() {\n"
     "    vec2 ndc = (pos / viewport) * 2.0 - 1.0;\n"
     "    ndc.y    = -ndc.y;\n"
     "    gl_Position = vec4(ndc, 0.0, 1.0);\n"
     "    v_uv     = uv;\n"
     "    v_color  = color;\n"
     "    v_weight = weight;\n"
     "}\n";

// atlas stores a signed distance field (0.5 == glyph edge), not plain
// coverage — fwidth() gives the AA band in SDF units for the *current*
// screen-space derivative, so the same atlas stays crisp whether the glyph
// is drawn tiny or blown up to 4K, with no re-bake needed per scale.
static const char *RENDER_TEXT_FRAG_SRC =
     "#version 460 core\n"
     "in vec2 v_uv;\n"
     "in vec4 v_color;\n"
     "in float v_weight;\n"
     "uniform sampler2D atlas;\n"
     "out vec4 frag_color;\n"
     "void main() {\n"
     "    float dist = texture(atlas, v_uv).r;\n"
     "    float aa   = max(fwidth(dist) * 0.75, 1e-4);\n"
     // v_weight (0 = regular, >0 = bold) shifts the SDF edge below 0.5 so
     // more of each glyph's distance field falls inside the fill, thickening
     // strokes without a second atlas/font.
     "    float edge = 0.5 - v_weight;\n"
     "    float a    = smoothstep(edge - aa, edge + aa, dist);\n"
     "    frag_color = vec4(v_color.rgb, v_color.a * a);\n"
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

static void render_text_upload_atlas(const simple_font *font) {
    simple_font_atlas atlas = simple_font_get_atlas(font);

    glBindTexture(GL_TEXTURE_2D, render_text_state.atlas_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas.width, atlas.height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, atlas.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    // LINEAR_MIPMAP_LINEAR for minification: draw scale is usually smaller
    // than the atlas's native raster size, and nearest-neighbor minification
    // just drops texels instead of averaging them down, losing detail.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_text_init(void) {
    GLuint vert = render_text_compile_shader(GL_VERTEX_SHADER, RENDER_TEXT_VERT_SRC);
    GLuint frag = render_text_compile_shader(GL_FRAGMENT_SHADER, RENDER_TEXT_FRAG_SRC);

    render_text_state.shader = glCreateProgram();
    glAttachShader(render_text_state.shader, vert);
    glAttachShader(render_text_state.shader, frag);
    glLinkProgram(render_text_state.shader);
    glDeleteShader(vert);
    glDeleteShader(frag);

    render_text_state.u_viewport =
         glGetUniformLocation(render_text_state.shader, "viewport");

    glGenVertexArrays(1, &render_text_state.vao);
    glGenBuffers(1, &render_text_state.vbo);
    glBindVertexArray(render_text_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_text_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(render_text_state.vertices), NULL,
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
         0, 2, GL_FLOAT, GL_FALSE, sizeof(render_text_vertex),
         (const void *)offsetof(render_text_vertex,  // NOLINT(performance-no-int-to-ptr)
                                x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
         1, 2, GL_FLOAT, GL_FALSE, sizeof(render_text_vertex),
         (const void *)offsetof(render_text_vertex,  // NOLINT(performance-no-int-to-ptr)
                                u));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
         2, 4, GL_FLOAT, GL_FALSE, sizeof(render_text_vertex),
         (const void *)offsetof(render_text_vertex,  // NOLINT(performance-no-int-to-ptr)
                                r));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
         3, 1, GL_FLOAT, GL_FALSE, sizeof(render_text_vertex),
         (const void *)offsetof(render_text_vertex,  // NOLINT(performance-no-int-to-ptr)
                                weight));
    glBindVertexArray(0);

    render_text_state.quad_count = 0;
    render_text_state.bound_font = NULL;
    render_text_state.dirty      = true;
    glGenTextures(1, &render_text_state.atlas_texture);

    log_debug("render_text.init: ready");
}

void render_text_destroy(void) {
    glDeleteProgram(render_text_state.shader);
    glDeleteVertexArrays(1, &render_text_state.vao);
    glDeleteBuffers(1, &render_text_state.vbo);
    glDeleteTextures(1, &render_text_state.atlas_texture);
}

void render_text_invalidate(void) { render_text_state.dirty = true; }

// Pure geometry, no GL/font/state dependency — trims [qx0,qy0]-[qx1,qy1] (with UV
// [u0,v0]-[u1,v1]) to the clip rect, re-interpolating UV at any edge it trims so a
// partial glyph still samples the right slice of the atlas instead of the whole
// glyph squashed into a smaller quad. Returns false (out params untouched) if the
// quad is fully outside the clip rect — caller should skip it entirely.
static bool render_text_clip_quad(float qx0, float qy0, float qx1, float qy1, float u0,
                                  float v0, float u1, float v1, float clip_x0,
                                  float clip_y0, float clip_x1, float clip_y1,
                                  float *out_x0, float *out_y0, float *out_x1,
                                  float *out_y1, float *out_u0, float *out_v0,
                                  float *out_u1, float *out_v1) {
    float cx0 = fmaxf(qx0, clip_x0);
    float cy0 = fmaxf(qy0, clip_y0);
    float cx1 = fminf(qx1, clip_x1);
    float cy1 = fminf(qy1, clip_y1);

    if (cx1 <= cx0 || cy1 <= cy0) return false;

    *out_x0 = cx0;
    *out_y0 = cy0;
    *out_x1 = cx1;
    *out_y1 = cy1;
    *out_u0 = u0 + (cx0 - qx0) / (qx1 - qx0) * (u1 - u0);
    *out_u1 = u0 + (cx1 - qx0) / (qx1 - qx0) * (u1 - u0);
    *out_v0 = v0 + (cy0 - qy0) / (qy1 - qy0) * (v1 - v0);
    *out_v1 = v0 + (cy1 - qy0) / (qy1 - qy0) * (v1 - v0);
    return true;
}

// clip_x0/y0/x1/y1 bound every glyph quad in caller space (pass +-INFINITY on all
// sides for "no clipping"). This is a CPU-side substitute for glScissor, so callers
// never need GL scissor state at all, at the cost of a few extra comparisons per
// glyph (render_text_clip_quad above).
static void render_text_draw_weighted_clipped(float x, float y, const char *str,
                                              float scale, color4f color,
                                              const simple_font *font, float weight,
                                              float clip_x0, float clip_y0, float clip_x1,
                                              float clip_y1) {
    if (font != render_text_state.bound_font || render_text_state.dirty) {
        render_text_upload_atlas(font);
        render_text_state.bound_font = font;
        render_text_state.dirty      = false;
    }

    simple_font_atlas atlas            = simple_font_get_atlas(font);
    int               fallback_advance = simple_font_get_advance(font);

    float cursor_x = x;
    for (const char *p = str; *p; p++) {
        const simple_font_glyph *g = simple_font_get_glyph(font, *p);
        if (g) {
            if (render_text_state.quad_count < RENDER_TEXT_MAX_QUADS) {
                float qx0 = cursor_x;
                float qy0 = y + (float)g->y_offset * scale;
                float qx1 = qx0 + (float)g->width * scale;
                float qy1 = qy0 + (float)g->height * scale;

                float u0 = (float)g->x / (float)atlas.width;
                float v0 = (float)g->y / (float)atlas.height;
                float u1 = (float)(g->x + g->width) / (float)atlas.width;
                float v1 = (float)(g->y + g->height) / (float)atlas.height;

                float cx0, cy0, cx1, cy1, cu0, cv0, cu1, cv1;
                if (render_text_clip_quad(qx0, qy0, qx1, qy1, u0, v0, u1, v1, clip_x0,
                                          clip_y0, clip_x1, clip_y1, &cx0, &cy0, &cx1,
                                          &cy1, &cu0, &cv0, &cu1, &cv1)) {
                    render_text_vertex *v =
                         &render_text_state
                               .vertices[(ptrdiff_t)render_text_state.quad_count *
                                         RENDER_TEXT_VERTS_PER_QUAD];

                    // clang-format off
                    v[0] = (render_text_vertex){cx0, cy0, cu0, cv0, color.r, color.g, color.b, color.a, weight};
                    v[1] = (render_text_vertex){cx1, cy0, cu1, cv0, color.r, color.g, color.b, color.a, weight};
                    v[2] = (render_text_vertex){cx1, cy1, cu1, cv1, color.r, color.g, color.b, color.a, weight};
                    v[3] = (render_text_vertex){cx0, cy0, cu0, cv0, color.r, color.g, color.b, color.a, weight};
                    v[4] = (render_text_vertex){cx1, cy1, cu1, cv1, color.r, color.g, color.b, color.a, weight};
                    v[5] = (render_text_vertex){cx0, cy1, cu0, cv1, color.r, color.g, color.b, color.a, weight};
                    // clang-format on

                    render_text_state.quad_count++;
                }
            }
            cursor_x += (float)g->advance * scale;
        } else {
            cursor_x += (float)fallback_advance * scale;
        }
    }
}

void render_text_draw_basic(float x, float y, const char *str, float scale, color4f color,
                            const simple_font *font, float weight) {
    render_text_draw_weighted_clipped(x, y, str, scale, color, font, weight, -INFINITY,
                                      -INFINITY, INFINITY, INFINITY);
}

void render_text_draw_clipped(float x, float y, const char *str, float scale,
                              color4f color, const simple_font *font, float weight,
                              float clip_x0, float clip_y0, float clip_x1,
                              float clip_y1) {
    render_text_draw_weighted_clipped(x, y, str, scale, color, font, weight, clip_x0,
                                      clip_y0, clip_x1, clip_y1);
}

void render_text_flush(void) {
    if (render_text_state.quad_count == 0) return;

    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    GLboolean prev_blend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(render_text_state.shader);
    glUniform2f(render_text_state.u_viewport, vw, vh);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_text_state.atlas_texture);
    glBindVertexArray(render_text_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, render_text_state.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    (GLsizeiptr)((size_t)render_text_state.quad_count *
                                 RENDER_TEXT_VERTS_PER_QUAD * sizeof(render_text_vertex)),
                    render_text_state.vertices);

    glDrawArrays(GL_TRIANGLES, 0,
                 render_text_state.quad_count * RENDER_TEXT_VERTS_PER_QUAD);

    glBindVertexArray(0);
    glUseProgram(0);

    if (!prev_blend) glDisable(GL_BLEND);

    render_text_state.quad_count = 0;
}

#else  // RENDER_BACKEND != RENDER_BACKEND_OPENGL — render_text not ported to this backend
       // yet

void render_text_init(void) {}
void render_text_destroy(void) {}
void render_text_draw_basic(float x, float y, const char *str, float scale, color4f color,
                            const simple_font *font, float weight) {
    (void)x;
    (void)y;
    (void)str;
    (void)scale;
    (void)color;
    (void)font;
    (void)weight;
}
void render_text_draw_clipped(float x, float y, const char *str, float scale,
                              color4f color, const simple_font *font, float weight,
                              float clip_x0, float clip_y0, float clip_x1,
                              float clip_y1) {
    (void)clip_x0;
    (void)clip_y0;
    (void)clip_x1;
    (void)clip_y1;
    render_text_draw_basic(x, y, str, scale, color, font, weight);
}
void render_text_flush(void) {}
void render_text_invalidate(void) {}

#endif

static float render_text_measure_width(const char *str, float scale,
                                       const simple_font *font) {
    int   fallback_advance = simple_font_get_advance(font);
    float width            = 0.0f;
    for (const char *p = str; *p; p++) {
        const simple_font_glyph *g = simple_font_get_glyph(font, *p);
        width += (float)(g ? g->advance : fallback_advance) * scale;
    }
    return width;
}

void render_text_draw_padded(float x, float y, const char *str, float scale,
                             color4f color, const simple_font *font, float pad_x,
                             float pad_y, float weight) {
    render_text_draw_basic(x + pad_x, y + pad_y, str, scale, color, font, weight);
}

void render_text_draw_margined(float x, float y, const char *str, float scale,
                               color4f color, const simple_font *font, float margin_x,
                               float margin_y, float weight) {
    float dx = x + margin_x;
    float dy = y + margin_y;

    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;
    float tw = render_text_measure_width(str, scale, font);
    float th = (float)SIMPLE_FONT_GLYPH_SIZE * scale;

    dx = fminf(dx, vw - margin_x - tw);
    dy = fminf(dy, vh - margin_y - th);

    render_text_draw_basic(dx, dy, str, scale, color, font, weight);
}

void render_text_draw_full(float x, float y, const char *str, float scale, color4f color,
                           const simple_font *font, float margin_x, float margin_y,
                           float pad_x, float pad_y, bool is_center, float weight) {
    float tw = render_text_measure_width(str, scale, font);
    float th = (float)SIMPLE_FONT_GLYPH_SIZE * scale;

    float vw = (float)g_ctx.window.width;
    float vh = (float)g_ctx.window.height;

    float dx = fminf(x + margin_x, vw - margin_x - tw);
    float dy = fminf(y + margin_y, vh - margin_y - th);

    dx += pad_x;
    dy += pad_y;

    if (is_center) dx -= tw / 2.0f;

    render_text_draw_basic(dx, dy, str, scale, color, font, weight);
}

#endif
