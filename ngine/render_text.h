#ifndef ZOD_NGINE_RENDER_TEXT_H
#define ZOD_NGINE_RENDER_TEXT_H

#include <modules/simple_font.h>

#include "common.h"

void render_text_init(const simple_font *font);
void render_text_destroy(void);
void render_text_draw(float x, float y, const char *str, float scale, color4f color);
void render_text_flush(void);

#endif
