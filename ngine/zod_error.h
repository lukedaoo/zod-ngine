#ifndef ZOD_ERROR_H
#define ZOD_ERROR_H

const char *zod_get_error(void);
void        zod_set_error(const char *fmt, ...);

#endif
