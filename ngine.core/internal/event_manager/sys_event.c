#ifdef ZOD_NGINE_IMPLEMENTATION

#include <ngine.lib/log.h>
#include "event_manager_internal.h"
#include "../engine_context/engine_context_internal.h"
#include "../render/render_internal.h"

// SYS_EVENT_ON_RESIZE_WINDOW
event_callback_result sys_event_priv_on_resize_window(event_context *ctx,
                                                      void          *userdata) {
    (void)userdata;
    const sys_event_resize_payload *payload =
         (const sys_event_resize_payload *)ctx->payload;
    int width  = payload->width;
    int height = payload->height;

    window *window = &g_ctx.window;
    if (width <= 0 || height <= 0)
        return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
    if (width == window->width && height == window->height)
        return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};

    window->width  = width;
    window->height = height;
    render_backend_priv_resize(window->backend.context, width, height);
    log_debug("sys_event.on_resize_window: %dx%d", width, height);
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}
// SYS_EVENT_ON_CONFIG_RELOAD_FULL
event_callback_result sys_event_priv_on_config_reload_full(event_context *ctx,
                                                           void          *userdata) {
    (void)ctx;
    (void)userdata;
    log_debug("sys_event.on_config_reload_full: fired");
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

// SYS_EVENT_ON_CONFIG_RELOAD_SINGLE
event_callback_result sys_event_priv_on_config_reload_single(event_context *ctx,
                                                             void          *userdata) {
    (void)userdata;
    log_debug("sys_event.on_config_reload_single: cvar='%s'", (const char *)ctx->payload);
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}
#endif
