#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../common.h"

color4f color4f_from_u32(uint32_t c) {
    color4f result;
    result.r = (float)((c >> 24) & 0xFF) / 255.0f;
    result.g = (float)((c >> 16) & 0xFF) / 255.0f;
    result.b = (float)((c >> 8) & 0xFF) / 255.0f;
    result.a = (float)((c >> 0) & 0xFF) / 255.0f;
    return result;
}

#endif
