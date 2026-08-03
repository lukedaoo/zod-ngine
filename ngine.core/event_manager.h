#ifndef ZOD_EVENT_MANAGER_H
#define ZOD_EVENT_MANAGER_H

#include <ngine.lib/event.h>
typedef struct event_manager event_manager;

// sys_event_ids range [1-100]
enum {
    SYS_EVENT_NO_OPT = 0,
    // window 1->10
    SYS_EVENT_ON_RESIZE_WINDOW = 1,
    // config 11 -> 20
    SYS_EVENT_ON_CONFIG_RELOAD_FULL   = 11,
    SYS_EVENT_ON_CONFIG_RELOAD_SINGLE = 12
} sys_event_ids;

typedef struct {
    int width;
    int height;
} sys_event_resize_payload;

void         event_manager_priv_init(event_manager *mgr);
void         event_manager_priv_destroy(event_manager *mgr);
event_handle event_manager_priv_subscribe(event_manager       *mgr,
                                          const event_category category,
                                          const int            event_id,
                                          const event_callback callback, void *userdata,
                                          const event_userdata_destroy destroy_fn);
bool event_manager_priv_unsubscribe_by_event_identifier(event_manager       *mgr,
                                                        const event_category category,
                                                        const int            event_id);
bool event_manager_priv_unsubscribe(event_manager *mgr, const event_handle handle);

void event_manager_priv_publish(event_manager *mgr, event_context *ctx);

#endif
