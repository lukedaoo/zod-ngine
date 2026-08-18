#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../keybind_manager.h"
#include "../../zod_ngine.h"
#include "../engine_context/engine_context_internal.h"

bool zngine_keybind_manager_load(const char *path, const keybind_vocab *vocab) {
    return keybind_manager_priv_load(&g_ctx.keybind_manager, path, vocab,
                                     &g_ctx.action_manager);
}

bool zngine_keybind_manager_merge(const char *path, const keybind_vocab *vocab) {
    return keybind_manager_priv_merge(&g_ctx.keybind_manager, path, vocab,
                                      &g_ctx.action_manager);
}

action_handle zngine_keybind_resolve(const action_mode context, const int key,
                                     const int trigger) {
    return keybind_manager_priv_resolve(&g_ctx.keybind_manager, context, key, trigger);
}

#endif
