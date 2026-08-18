#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/action.h>
#include "../../input.h"
#include "action_manager_internal.h"

static bool action_manager_key_trigger_fired(const action_trigger_type type,
                                             const int key, void *trigger_ctx) {
    (void)trigger_ctx;
    switch ((action_key_trigger)type) {
        case ACTION_TRIGGER_KEY_PRESSED:
            return input_priv_key_pressed((zod_key_t)key);
        case ACTION_TRIGGER_KEY_DOWN:
            return input_priv_key_down((zod_key_t)key);
        case ACTION_TRIGGER_KEY_RELEASED:
            return input_priv_key_released((zod_key_t)key);
        default:
            return false;
    }
}

void action_manager_priv_init(action_manager *mgr) { action_init(&mgr->table); }
void action_manager_priv_destroy(action_manager *mgr) { action_destroy(&mgr->table); }

const action_table *action_manager_priv_table(const action_manager *mgr) {
    return mgr ? &mgr->table : NULL;
}

action_handle action_manager_priv_resolve_by_name(const action_manager     *mgr,
                                                  const action_mode         context,
                                                  const action_trigger_type type,
                                                  const char               *name) {
    if (!mgr) return ACTION_HANDLE_INVALID;
    return action_resolve_by_name(&mgr->table, context, type, name);
}

action_handle action_manager_priv_resolve_by_key(const action_manager     *mgr,
                                                 const action_mode         context,
                                                 const action_trigger_type type,
                                                 const int                 key) {
    if (!mgr) return ACTION_HANDLE_INVALID;
    return action_resolve_by_key(&mgr->table, context, type, key);
}

action_handle action_manager_priv_bind(action_manager *mgr, const action_mode context,
                                       const action_trigger_type type, const int key,
                                       const char *name, action_executor *executor) {
    if (!mgr) return ACTION_HANDLE_INVALID;
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

action_execute_result action_manager_priv_execute(action_manager     *mgr,
                                                  const action_handle handle,
                                                  void               *userdata) {
    if (!mgr) return (action_execute_result){.type = ACTION_EXECUTE_RESULT_VOID};
    return action_execute(&mgr->table, handle, userdata);
}

size_t action_manager_priv_dispatch(action_manager *mgr, const action_mode context,
                                    void *userdata) {
    if (!mgr) return 0;
    return action_dispatch(&mgr->table, context, action_manager_key_trigger_fired, NULL,
                           userdata);
}

action_execute_result action_manager_priv_execute_by_name(action_manager   *mgr,
                                                          const action_mode context,
                                                          const action_trigger_type type,
                                                          const char               *name,
                                                          void *userdata) {
    return action_execute_by_name(&mgr->table, context, type, name, userdata);
}
#endif
