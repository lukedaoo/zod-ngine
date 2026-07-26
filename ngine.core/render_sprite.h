#ifndef ZOD_NGINE_RENDER_SPRITE_H
#define ZOD_NGINE_RENDER_SPRITE_H

#include <stdint.h>

#include <ngine.lib/types.h>

typedef struct sprite_texture {
    uint32_t id;  // GL texture name; 0 == invalid/failed-to-load
    int      width;
    int      height;
} sprite_texture;

sprite_texture sprite_texture_load(const char *path);
sprite_texture sprite_texture_solid(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void sprite_texture_unload(sprite_texture *tex);

void render_sprite_init(void);
void render_sprite_destroy(void);

void render_sprite_draw(sprite_texture tex, float x, float y, float w, float h,
                        color4f tint);
void render_sprite_flush(void);

#endif
