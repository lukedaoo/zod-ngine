#ifndef ZOD_NGINE_RENDER_TEXT_H
#define ZOD_NGINE_RENDER_TEXT_H

#include <ngine.lib/simple_font.h>
#include <ngine.lib/types.h>

void render_text_init(void);
void render_text_destroy(void);
void render_text_draw_basic(float x, float y, const char *str, float scale, color4f color,
                            const simple_font *font);

void render_text_draw_padded(float x, float y, const char *str, float scale,
                             color4f color, const simple_font *font, float pad_x,
                             float pad_y);

void render_text_draw_margined(float x, float y, const char *str, float scale,
                               color4f color, const simple_font *font, float margin_x,
                               float margin_y);

void render_text_draw_full(float x, float y, const char *str, float scale, color4f color,
                           const simple_font *font, float margin_x, float margin_y,
                           float pad_x, float pad_y, bool is_center);

void render_text_flush(void);

// Call after reloading/mutating a simple_font in place (same pointer, new content)
// so the next render_text_draw_basic re-uploads its atlas instead of assuming the GPU
// copy is still current.
void render_text_invalidate(void);

#endif
