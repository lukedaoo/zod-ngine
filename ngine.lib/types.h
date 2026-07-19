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
