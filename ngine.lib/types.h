#ifndef TYPES_H
#define TYPES_H
#include <inttypes.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;
typedef size_t   usize;

typedef struct {
    float r, g, b, a;
} color4f;

#define COLOR4F_WHITE   ((color4f){.r = 1.00f, .g = 1.00f, .b = 1.00f, .a = 1.0f})
#define COLOR4F_BLACK   ((color4f){.r = 0.00f, .g = 0.00f, .b = 0.00f, .a = 1.0f})
#define COLOR4F_GRAY    ((color4f){.r = 0.55f, .g = 0.55f, .b = 0.55f, .a = 1.0f})
#define COLOR4F_RED     ((color4f){.r = 0.90f, .g = 0.20f, .b = 0.20f, .a = 1.0f})
#define COLOR4F_GREEN   ((color4f){.r = 0.20f, .g = 0.85f, .b = 0.20f, .a = 1.0f})
#define COLOR4F_YELLOW  ((color4f){.r = 0.90f, .g = 0.80f, .b = 0.10f, .a = 1.0f})
#define COLOR4F_CYAN    ((color4f){.r = 0.20f, .g = 0.80f, .b = 0.80f, .a = 1.0f})
#define COLOR4F_MAGENTA ((color4f){.r = 0.85f, .g = 0.20f, .b = 0.85f, .a = 1.0f})

color4f color4f_from_u32(uint32_t c);

#ifdef TYPES_IMPLEMENTATION

color4f color4f_from_u32(uint32_t c) {
    color4f result;
    result.r = (float)((c >> 24) & 0xFF) / 255.0f;
    result.g = (float)((c >> 16) & 0xFF) / 255.0f;
    result.b = (float)((c >> 8) & 0xFF) / 255.0f;
    result.a = (float)((c >> 0) & 0xFF) / 255.0f;
    return result;
}

#endif  // TYPES_IMPLEMENTATION
#endif  // TYPES_H
