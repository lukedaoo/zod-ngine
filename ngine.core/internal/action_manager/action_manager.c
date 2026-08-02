#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/action.h>
#include "action_manager_internal.h"

void action_manager_priv_init(action_manager *mgr) { action_init(&mgr->table); }
void action_manager_priv_destroy(action_manager *mgr) { action_destroy(&mgr->table); }

action *action_manager_priv_resolve_by_name(const action_manager     *mgr,
                                            const action_mode         context,
                                            const action_trigger_type type,
                                            const char               *name) {
    return action_resolve_by_name(&mgr->table, context, type, name);
}

action *action_manager_priv_resolve_by_key(const action_manager     *mgr,
                                           const action_mode         context,
                                           const action_trigger_type type,
                                           const int                 key) {
    return action_resolve_by_key(&mgr->table, context, type, key);
}

bool action_manager_priv_bind(action_manager *mgr, const action_mode context,
                              const action_trigger_type type, const int key,
                              const char *name, action_executor *executor) {
    return action_bind(&mgr->table, context, type, key, name, executor);
}

bool action_manager_priv_rebind(action_manager *mgr, const action_mode context,
                                const action_trigger_type type, const int old_key,
                                const int new_key) {
    return action_rebind(&mgr->table, context, type, old_key, new_key);
}

bool action_manager_priv_unbind_by_key(action_manager *mgr, const action_mode context,
                                       const action_trigger_type type, const int key) {
    return action_unbind_by_key(&mgr->table, context, type, key);
}

bool action_manager_priv_unbind_by_name(action_manager *mgr, const action_mode context,
                                        const action_trigger_type type,
                                        const char               *name) {
    return action_unbind_by_name(&mgr->table, context, type, name);
}

bool action_manager_priv_unbind_all(action_manager *mgr, const action_mode context,
                                    const action_trigger_type type) {
    return action_unbind_all(&mgr->table, context, type);
}

action_execute_result action_manager_priv_execute(action *action, void *userdata) {
    return action_execute(action, userdata);
}

action_execute_result action_manager_priv_execute_by_name(action_manager   *mgr,
                                                          const action_mode context,
                                                          const action_trigger_type type,
                                                          const char               *name,
                                                          void *userdata) {
    return action_execute_by_name(&mgr->table, context, type, name, userdata);
}
#endif
