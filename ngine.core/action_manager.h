#ifndef ZOD_ACTION_MANAGER_H
#define ZOD_ACTION_MANAGER_H

#include <ngine.lib/action.h>
typedef struct action_manager action_manager;

typedef enum {
    ACTION_TRIGGER_KEY_PRESSED = 0,
    ACTION_TRIGGER_KEY_DOWN,
    ACTION_TRIGGER_KEY_RELEASED,
} action_key_trigger;

void action_manager_priv_init(action_manager *mgr);
void action_manager_priv_destroy(action_manager *mgr);

const action_table *action_manager_priv_table(const action_manager *mgr);

action_handle action_manager_priv_resolve_by_name(const action_manager     *mgr,
                                                  const action_mode         context,
                                                  const action_trigger_type type,
                                                  const char               *name);
action_handle action_manager_priv_resolve_by_key(const action_manager     *mgr,
                                                 const action_mode         context,
                                                 const action_trigger_type type,
                                                 const int                 key);

action_handle action_manager_priv_bind(action_manager *mgr, const action_mode context,
                                       const action_trigger_type type, const int key,
                                       const char *name, action_executor *executor);

bool action_manager_priv_rebind(action_manager *mgr, const action_mode context,
                                const action_trigger_type type, const int old_key,
                                const int new_key);

bool action_manager_priv_unbind_by_key(action_manager *mgr, const action_mode context,
                                       const action_trigger_type type, const int key);
bool action_manager_priv_unbind_by_name(action_manager *mgr, const action_mode context,
                                        const action_trigger_type type, const char *name);
bool action_manager_priv_unbind_all(action_manager *mgr, const action_mode context,
                                    const action_trigger_type type);

action_execute_result action_manager_priv_execute(action_manager     *mgr,
                                                  const action_handle handle,
                                                  void               *userdata);

size_t action_manager_priv_dispatch(action_manager *mgr, const action_mode context,
                                    void *userdata);

action_execute_result action_manager_priv_execute_by_name(action_manager   *mgr,
                                                          const action_mode context,
                                                          const action_trigger_type type,
                                                          const char               *name,
                                                          void *userdata);

#endif
