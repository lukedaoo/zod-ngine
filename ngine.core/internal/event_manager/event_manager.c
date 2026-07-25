#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/event.h>
#include "event_manager_internal.h"

void event_manager_priv_init(event_manager *mgr) {
    event_table_init(&mgr->table);
    event_manager_priv_subscribe_sys_events(mgr);
}
void event_manager_priv_destroy(event_manager *mgr) { event_table_destroy(&mgr->table); }

bool event_manager_priv_subscribe(event_manager *mgr, const event_category category,
                                  const int event_id, const event_callback callback,
                                  void                        *userdata,
                                  const event_userdata_destroy destroy_fn) {
    return event_table_subscribe(&mgr->table, category, event_id, callback, userdata,
                                 destroy_fn);
}

bool event_manager_priv_unsubscribe(event_manager *mgr, const event_category category,
                                    const int event_id, const event_callback callback,
                                    void *userdata) {
    return event_table_unsubscribe(&mgr->table, category, event_id, callback, userdata);
}

bool event_manager_priv_unsubscribe_by_event_identifier(event_manager       *mgr,
                                                        const event_category category,
                                                        const int            event_id) {
    return event_table_unsubscribe_by_event_identifier(&mgr->table, category, event_id);
}

void event_manager_priv_publish(event_manager *mgr, event_context *ctx) {
    event_table_publish(&mgr->table, ctx);
}

void event_manager_priv_subscribe_sys_events(event_manager *mgr) {
    event_table_subscribe(&mgr->table, EVENT_TAG_SYSTEM_WINDOW,
                          SYS_EVENT_ON_RESIZE_WINDOW, sys_event_priv_on_resize_window,
                          NULL, NULL);
    event_table_subscribe(&mgr->table, EVENT_TAG_SYSTEM_CONFIG,
                          SYS_EVENT_ON_CONFIG_RELOAD_FULL,
                          sys_event_priv_on_config_reload_full, NULL, NULL);
    event_table_subscribe(&mgr->table, EVENT_TAG_SYSTEM_CONFIG,
                          SYS_EVENT_ON_CONFIG_RELOAD_SINGLE,
                          sys_event_priv_on_config_reload_single, NULL, NULL);
}
#endif
