#ifndef ZOD_NGINE_RENDER_TEXT_H
#define ZOD_NGINE_RENDER_TEXT_H

#include <modules/simple_font.h>

#include "common.h"

void render_text_init(void);
void render_text_destroy(void);
void render_text_draw(float x, float y, const char *str, float scale, color4f color,
                      const simple_font *font);
void render_text_flush(void);

// Call after reloading/mutating a simple_font in place (same pointer, new content)
// so the next render_text_draw re-uploads its atlas instead of assuming the GPU
// copy is still current.
void render_text_invalidate(void);

#endif
