#ifndef EVENT_MANAGER_INTERNAL_H
#define EVENT_MANAGER_INTERNAL_H

#include <ngine.lib/event_dispatcher.h>
#include "../../event_manager.h"

struct event_manager {
    event_dispatcher dispatcher;
};

void event_manager_priv_subscribe_sys_events(event_manager *mgr);

// System events
event_callback_result sys_event_priv_on_resize_window(event_context *ctx, void *userdata);
event_callback_result sys_event_priv_on_config_reload_full(event_context *ctx,
                                                           void          *userdata);
event_callback_result sys_event_priv_on_config_reload_single(event_context *ctx,
                                                             void          *userdata);

#endif
