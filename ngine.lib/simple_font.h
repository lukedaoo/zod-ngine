#ifndef SIMPLE_FONT_H
#define SIMPLE_FONT_H

#include <stdint.h>

#define SIMPLE_FONT_FIRST_CHAR  32
#define SIMPLE_FONT_LAST_CHAR   126
#define SIMPLE_FONT_GLYPH_COUNT (SIMPLE_FONT_LAST_CHAR - SIMPLE_FONT_FIRST_CHAR + 1)

#define SIMPLE_FONT_GLYPH_SIZE 8

#ifndef SIMPLE_FONT_ATLAS_COLS
#define SIMPLE_FONT_ATLAS_COLS 16
#endif

#ifndef SIMPLE_FONT_ATLAS_ROWS
#define SIMPLE_FONT_ATLAS_ROWS 6
#endif

#define SIMPLE_FONT_ATLAS_WIDTH  (SIMPLE_FONT_ATLAS_COLS * SIMPLE_FONT_GLYPH_SIZE)
#define SIMPLE_FONT_ATLAS_HEIGHT (SIMPLE_FONT_ATLAS_ROWS * SIMPLE_FONT_GLYPH_SIZE)

static_assert(SIMPLE_FONT_ATLAS_COLS * SIMPLE_FONT_ATLAS_ROWS >= SIMPLE_FONT_GLYPH_COUNT,
              "simple_font: atlas grid too small for glyph count");

#ifndef SIMPLE_FONT_TTF_PIXEL_SIZE
#define SIMPLE_FONT_TTF_PIXEL_SIZE 32
#endif

// Glyphs are baked as a signed distance field (stbtt_GetCodepointSDF)
// instead of a plain coverage bitmap — the atlas can then be sampled and
// thresholded at any draw scale (tiny or 4K) with a scale-aware AA band in
// the fragment shader, instead of blurring/aliasing past the bake size.
#ifndef SIMPLE_FONT_SDF_PADDING
#define SIMPLE_FONT_SDF_PADDING 6
#endif

#ifndef SIMPLE_FONT_SDF_ONEDGE_VALUE
#define SIMPLE_FONT_SDF_ONEDGE_VALUE 128
#endif

#define SIMPLE_FONT_SDF_PIXEL_DIST_SCALE \
    ((float)SIMPLE_FONT_SDF_ONEDGE_VALUE / (float)SIMPLE_FONT_SDF_PADDING)

#ifndef SIMPLE_FONT_TTF_ATLAS_COLS
#define SIMPLE_FONT_TTF_ATLAS_COLS 16
#endif

#ifndef SIMPLE_FONT_TTF_ATLAS_ROWS
#define SIMPLE_FONT_TTF_ATLAS_ROWS 6
#endif

#define SIMPLE_FONT_TTF_ATLAS_WIDTH \
    (SIMPLE_FONT_TTF_ATLAS_COLS * SIMPLE_FONT_TTF_PIXEL_SIZE)
#define SIMPLE_FONT_TTF_ATLAS_HEIGHT \
    (SIMPLE_FONT_TTF_ATLAS_ROWS * SIMPLE_FONT_TTF_PIXEL_SIZE)

static_assert(SIMPLE_FONT_TTF_ATLAS_COLS * SIMPLE_FONT_TTF_ATLAS_ROWS >=
                   SIMPLE_FONT_GLYPH_COUNT,
              "simple_font: ttf atlas grid too small for glyph count");

typedef struct simple_font_glyph {
    uint16_t x;
    uint16_t y;
    uint8_t  width;
    uint8_t  height;
    uint8_t  advance;
    int16_t  y_offset;
} simple_font_glyph;

typedef struct simple_font_ascii {
    uint8_t           atlas[SIMPLE_FONT_ATLAS_WIDTH * SIMPLE_FONT_ATLAS_HEIGHT];
    int               atlas_width;
    int               atlas_height;
    simple_font_glyph glyphs[SIMPLE_FONT_GLYPH_COUNT];
} simple_font_ascii;

typedef struct simple_font_ttf {
    uint8_t           atlas[SIMPLE_FONT_TTF_ATLAS_WIDTH * SIMPLE_FONT_TTF_ATLAS_HEIGHT];
    int               atlas_width;
    int               atlas_height;
    simple_font_glyph glyphs[SIMPLE_FONT_GLYPH_COUNT];
    int               baseline;  // px above the top of a SIMPLE_FONT_TTF_PIXEL_SIZE
                                  // cell where the font's baseline sits — glyph
                                  // y_offset is baseline-relative, so callers need
                                  // this to place text by its visual top instead
} simple_font_ttf;

typedef enum simple_font_backend {
    SIMPLE_FONT_BACKEND_ASCII,
    SIMPLE_FONT_BACKEND_TTF,
} simple_font_backend;

typedef struct simple_font {
    simple_font_backend backend;
    union {
        simple_font_ascii ascii;
        simple_font_ttf   ttf;
    };
} simple_font;

typedef struct simple_font_atlas {
    const uint8_t *pixels;
    int            width;
    int            height;
} simple_font_atlas;

simple_font              simple_font_load(const char *path);
const simple_font_glyph *simple_font_get_glyph(const simple_font *font, char c);
simple_font_atlas        simple_font_get_atlas(const simple_font *font);
int                      simple_font_get_advance(const simple_font *font);

// Px above the top of a glyph's cell (at the font's native rasterization
// size) where the baseline sits. 0 for the ascii backend (its glyph
// y_offset is always 0, so callers already treat y as the visual top); the
// ttf backend's y_offset is baseline-relative, so callers need this to
// convert a desired visual-top y into the baseline y render_text expects.
int simple_font_get_baseline(const simple_font *font);

#ifdef SIMPLE_FONT_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../thirdparty/stb_truetype.h"

// clang-format off
// font8x8_basic (public domain, dhepper/font8x8), codepoints 0x20-0x7E.
// Each row is 8 bytes, one byte per pixel-row, LSB = leftmost pixel.
static const uint8_t simple_font_ascii_bitmap[SIMPLE_FONT_GLYPH_COUNT][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // ' '
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // '!'
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // '"'
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},   // '#'
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},   // '$'
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},   // '%'
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},   // '&'
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},   // '''
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},   // '('
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},   // ')'
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // '*'
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},   // '+'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // ','
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},   // '-'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // '.'
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},   // '/'
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // '0'
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // '1'
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // '2'
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // '3'
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // '4'
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // '5'
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // '6'
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // '7'
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // '8'
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // '9'
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},   // ':'
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},   // ';'
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},   // '<'
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},   // '='
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},   // '>'
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},   // '?'
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},   // '@'
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},   // 'A'
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},   // 'B'
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},   // 'C'
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},   // 'D'
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},   // 'E'
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},   // 'F'
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},   // 'G'
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},   // 'H'
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // 'I'
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},   // 'J'
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},   // 'K'
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},   // 'L'
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},   // 'M'
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},   // 'N'
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},   // 'O'
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},   // 'P'
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},   // 'Q'
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},   // 'R'
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},   // 'S'
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // 'T'
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},   // 'U'
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // 'V'
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},   // 'W'
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},   // 'X'
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},   // 'Y'
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},   // 'Z'
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},   // '['
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},   // '\'
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},   // ']'
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},   // '^'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // '_'
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // '`'
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},   // 'a'
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},   // 'b'
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},   // 'c'
    { 0x38, 0x30, 0x30, 0x3E, 0x33, 0x33, 0x6E, 0x00},   // 'd'
    { 0x00, 0x00, 0x1E, 0x33, 0x3F, 0x03, 0x1E, 0x00},   // 'e'
    { 0x1C, 0x36, 0x06, 0x0F, 0x06, 0x06, 0x0F, 0x00},   // 'f'
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // 'g'
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},   // 'h'
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // 'i'
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},   // 'j'
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},   // 'k'
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},   // 'l'
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},   // 'm'
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},   // 'n'
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},   // 'o'
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},   // 'p'
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},   // 'q'
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},   // 'r'
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},   // 's'
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},   // 't'
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},   // 'u'
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},   // 'v'
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},   // 'w'
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},   // 'x'
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},   // 'y'
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},   // 'z'
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},   // '{'
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // '|'
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},   // '}'
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // '~'
};
// clang-format on

static void simple_font_load_ascii(simple_font_ascii *font) {
    memset(font->atlas, 0, sizeof(font->atlas));

    for (int i = 0; i < SIMPLE_FONT_GLYPH_COUNT; i++) {
        int ox = (i % SIMPLE_FONT_ATLAS_COLS) * SIMPLE_FONT_GLYPH_SIZE;
        int oy = (i / SIMPLE_FONT_ATLAS_COLS) * SIMPLE_FONT_GLYPH_SIZE;

        for (int py = 0; py < SIMPLE_FONT_GLYPH_SIZE; py++) {
            uint8_t bits = simple_font_ascii_bitmap[i][py];
            for (int px = 0; px < SIMPLE_FONT_GLYPH_SIZE; px++) {
                uint8_t on = (bits >> px) & 1;
                font->atlas[(oy + py) * SIMPLE_FONT_ATLAS_WIDTH + (ox + px)] =
                     on ? 255 : 0;
            }
        }

        font->glyphs[i] = (simple_font_glyph){
             .x        = (uint16_t)ox,
             .y        = (uint16_t)oy,
             .width    = SIMPLE_FONT_GLYPH_SIZE,
             .height   = SIMPLE_FONT_GLYPH_SIZE,
             .advance  = SIMPLE_FONT_GLYPH_SIZE,
             .y_offset = 0,
        };
    }

    font->atlas_width  = SIMPLE_FONT_ATLAS_WIDTH;
    font->atlas_height = SIMPLE_FONT_ATLAS_HEIGHT;
}

static uint8_t *simple_font_read_file(const char *path, long *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long len = ftell(f);
    if (len < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    uint8_t *buf = malloc((size_t)len);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (n != (size_t)len) {
        free(buf);
        return NULL;
    }

    *out_size = len;
    return buf;
}

static bool simple_font_load_ttf(simple_font_ttf *font, const char *path) {
    long     size = 0;
    uint8_t *data = simple_font_read_file(path, &size);
    if (!data) return false;

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, data, stbtt_GetFontOffsetForIndex(data, 0))) {
        free(data);
        return false;
    }

    memset(font->atlas, 0, sizeof(font->atlas));
    float scale = stbtt_ScaleForPixelHeight(&info, SIMPLE_FONT_TTF_PIXEL_SIZE);

    int ascent = 0;
    stbtt_GetFontVMetrics(&info, &ascent, NULL, NULL);
    int baseline = (int)(ascent * scale + 0.5f);
    if (baseline < 0) baseline = 0;
    if (baseline > SIMPLE_FONT_TTF_PIXEL_SIZE - 1)
        baseline = SIMPLE_FONT_TTF_PIXEL_SIZE - 1;

    for (int i = 0; i < SIMPLE_FONT_GLYPH_COUNT; i++) {
        int ox = (i % SIMPLE_FONT_TTF_ATLAS_COLS) * SIMPLE_FONT_TTF_PIXEL_SIZE;
        int oy = (i / SIMPLE_FONT_TTF_ATLAS_COLS) * SIMPLE_FONT_TTF_PIXEL_SIZE;

        int            cw = 0, ch = 0, xoff = 0, yoff = 0;
        unsigned char *bitmap = stbtt_GetCodepointSDF(
             &info, scale, SIMPLE_FONT_FIRST_CHAR + i, SIMPLE_FONT_SDF_PADDING,
             SIMPLE_FONT_SDF_ONEDGE_VALUE, SIMPLE_FONT_SDF_PIXEL_DIST_SCALE, &cw, &ch,
             &xoff, &yoff);

        int gw = 0, gh = 0, cell_x = 0, cell_y = 0;
        if (bitmap) {
            gw = cw < SIMPLE_FONT_TTF_PIXEL_SIZE ? cw : SIMPLE_FONT_TTF_PIXEL_SIZE;

            cell_y = baseline + yoff;
            if (cell_y < 0) cell_y = 0;
            gh = ch;
            if (cell_y + gh > SIMPLE_FONT_TTF_PIXEL_SIZE)
                gh = SIMPLE_FONT_TTF_PIXEL_SIZE - cell_y;
            if (gh < 0) gh = 0;

            cell_x = xoff;
            if (cell_x < 0) cell_x = 0;
            if (cell_x + gw > SIMPLE_FONT_TTF_PIXEL_SIZE)
                gw = SIMPLE_FONT_TTF_PIXEL_SIZE - cell_x;
            if (gw < 0) gw = 0;

            for (int py = 0; py < gh; py++) {
                memcpy(&font->atlas[(oy + cell_y + py) * SIMPLE_FONT_TTF_ATLAS_WIDTH +
                                    (ox + cell_x)],
                       &bitmap[py * cw], (size_t)gw);
            }
            stbtt_FreeSDF(bitmap, NULL);
        }

        int advance_width = 0;
        stbtt_GetCodepointHMetrics(&info, SIMPLE_FONT_FIRST_CHAR + i, &advance_width,
                                   NULL);
        int advance = (int)(advance_width * scale + 0.5f);
        if (advance < 0) advance = 0;
        if (advance > 255) advance = 255;

        font->glyphs[i] = (simple_font_glyph){
             .x        = (uint16_t)(ox + cell_x),
             .y        = (uint16_t)(oy + cell_y),
             .width    = (uint8_t)gw,
             .height   = (uint8_t)gh,
             .advance  = (uint8_t)advance,
             .y_offset = (int16_t)yoff,
        };
    }

    font->atlas_width  = SIMPLE_FONT_TTF_ATLAS_WIDTH;
    font->atlas_height = SIMPLE_FONT_TTF_ATLAS_HEIGHT;
    font->baseline     = baseline;

    free(data);
    return true;
}

simple_font simple_font_load(const char *path) {
    simple_font font;

    if (path && simple_font_load_ttf(&font.ttf, path)) {
        font.backend = SIMPLE_FONT_BACKEND_TTF;
        return font;
    }

    simple_font_load_ascii(&font.ascii);
    font.backend = SIMPLE_FONT_BACKEND_ASCII;
    return font;
}

const simple_font_glyph *simple_font_get_glyph(const simple_font *font, char c) {
    if (c < SIMPLE_FONT_FIRST_CHAR || c > SIMPLE_FONT_LAST_CHAR) return NULL;
    int i = c - SIMPLE_FONT_FIRST_CHAR;
    return font->backend == SIMPLE_FONT_BACKEND_TTF ? &font->ttf.glyphs[i]
                                                    : &font->ascii.glyphs[i];
}

simple_font_atlas simple_font_get_atlas(const simple_font *font) {
    if (font->backend == SIMPLE_FONT_BACKEND_TTF) {
        return (simple_font_atlas){
             .pixels = font->ttf.atlas,
             .width  = font->ttf.atlas_width,
             .height = font->ttf.atlas_height,
        };
    }
    return (simple_font_atlas){
         .pixels = font->ascii.atlas,
         .width  = font->ascii.atlas_width,
         .height = font->ascii.atlas_height,
    };
}

int simple_font_get_advance(const simple_font *font) {
    return font->backend == SIMPLE_FONT_BACKEND_TTF ? SIMPLE_FONT_TTF_PIXEL_SIZE
                                                    : SIMPLE_FONT_GLYPH_SIZE;
}

int simple_font_get_baseline(const simple_font *font) {
    return font->backend == SIMPLE_FONT_BACKEND_TTF ? font->ttf.baseline : 0;
}

#endif
#endif
