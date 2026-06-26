#ifndef ZOD_NGINE_H
#define ZOD_NGINE_H

typedef struct zod_engine_dispatch    zod_engine_dispatch;
typedef struct zod_engine_init_params zod_engine_init_params;

bool zod_ngine_init(const zod_engine_init_params params);
void zod_ngine_destroy(void);

#endif
