#ifndef ZOD_NGINE_COMMON_H
#define ZOD_NGINE_COMMON_H

#include <stdint.h>

typedef struct {
    float r, g, b, a;
} color4f;

color4f color4f_from_u32(uint32_t c);

#endif
