#ifdef ZOD_NGINE_IMPLEMENTATION

#include "../../zod_ngine.h"
#include "../../action_manager.h"
#include "../engine_context/engine_context_internal.h"

action_handle zngine_action_resolve_by_name(const action_mode         context,
                                            const action_trigger_type type,
                                            const char               *name) {
    return action_manager_priv_resolve_by_name(&g_ctx.action_manager, context, type,
                                               name);
}

action_handle zngine_action_resolve_by_key(const action_mode         context,
                                           const action_trigger_type type,
                                           const int                 key) {
    return action_manager_priv_resolve_by_key(&g_ctx.action_manager, context, type, key);
}

action_handle zngine_action_bind(const action_mode         context,
                                 const action_trigger_type type, const int key,
                                 const char *name, action_executor *executor) {
    return action_manager_priv_bind(&g_ctx.action_manager, context, type, key, name,
                                    executor);
}

bool zngine_action_rebind(const action_mode context, const action_trigger_type type,
                          const int old_key, const int new_key) {
    return action_manager_priv_rebind(&g_ctx.action_manager, context, type, old_key,
                                      new_key);
}

bool zngine_action_unbind_by_key(const action_mode         context,
                                 const action_trigger_type type, const int key) {
    return action_manager_priv_unbind_by_key(&g_ctx.action_manager, context, type, key);
}

bool zngine_action_unbind_by_name(const action_mode         context,
                                  const action_trigger_type type, const char *name) {
    return action_manager_priv_unbind_by_name(&g_ctx.action_manager, context, type, name);
}

bool zngine_action_unbind_all(const action_mode context, const action_trigger_type type) {
    return action_manager_priv_unbind_all(&g_ctx.action_manager, context, type);
}

action_execute_result zngine_action_execute(const action_handle handle, void *userdata) {
    return action_manager_priv_execute(&g_ctx.action_manager, handle, userdata);
}

action_execute_result zngine_action_execute_by_name(const action_mode         context,
                                                    const action_trigger_type type,
                                                    const char *name, void *userdata) {
    return action_manager_priv_execute_by_name(&g_ctx.action_manager, context, type, name,
                                               userdata);
}

#endif
