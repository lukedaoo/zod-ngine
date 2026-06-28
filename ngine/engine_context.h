#ifndef ZOD_NGINE_ENGINE_CONTEXT_H
#define ZOD_NGINE_ENGINE_CONTEXT_H

typedef struct engine_context engine_context;

void engine_context_destroy(void);

extern engine_context g_ctx;

#endif
