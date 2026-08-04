#ifndef EVENT_H
#define EVENT_H

#include <stddef.h>
#include <stdbool.h>

typedef struct event_table event_table;

typedef int event_category;

typedef enum {
    EVENT_CALLBACK_RESULT_VOID,
    EVENT_CALLBACK_RESULT_INT,
    EVENT_CALLBACK_RESULT_FLOAT,
    EVENT_CALLBACK_RESULT_STRING,
    EVENT_CALLBACK_RESULT_PTR,
    EVENT_CALLBACK_RESULT_ERROR,
} event_callback_result_type;

typedef struct {
    event_category category;
    int            event_id;
} event_identifier;

typedef struct {
    event_identifier identifier;

    void  *payload;
    size_t payload_size;
} event_context;

typedef struct {
    event_callback_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
} event_callback_result;

// userdata: caller-owned context handed back unchanged on every invocation.
// event system only stores and forwards the pointer — never allocates,
// frees, or dereferences it. Caller must outlive the subscription (or
// unsubscribe before freeing it).
typedef event_callback_result (*event_callback)(event_context *ctx, void *userdata);
// destroy_fn: optional, called exactly once when a listener is removed
// (unsubscribe or table destroy) — pass NULL to keep borrowing userdata
// (caller retains ownership, default). Non-NULL transfers ownership to the
// event system for that one subscription only.
typedef void (*event_userdata_destroy)(void *userdata);

// @info: opaque handle to one subscription. EVENT_HANDLE_INVALID (0) is never
// a valid handle, so a zero-initialised handle is invalid by default.
typedef unsigned int event_handle;
#define EVENT_HANDLE_INVALID 0u

void event_table_init(event_table *event_table);
void event_table_destroy(event_table *event_table);
// @info: returns EVENT_HANDLE_INVALID on bad arguments or allocation failure.
event_handle event_table_subscribe(event_table *event_table,
                                   const event_category category, const int event_id,
                                   const event_callback callback, void *userdata,
                                   const event_userdata_destroy destroy_fn);
bool event_table_unsubscribe(event_table *event_table, const event_handle handle);
bool event_table_unsubscribe_by_event_identifier(event_table         *event_table,
                                                 const event_category category,
                                                 const int            event_id);
void event_table_publish(event_table *event_table, event_context *ctx);

#ifdef EVENT_IMPLEMENTATION

#ifndef EVENT_LOG_ENABLED
#define EVENT_LOG_ENABLED 0
#endif

#ifndef EVENT_DEFAULT_LISTENER_CAPACITY
#define EVENT_DEFAULT_LISTENER_CAPACITY 16
#endif

#include "collections/array_list.h"
#include "log.h"

typedef struct {
    event_identifier       identifier;
    event_callback         callback;
    void                  *listener_data;
    event_userdata_destroy destroy_fn;
} event_listener;

static void event_listener_release(event_listener *l) {
    if (l->destroy_fn) l->destroy_fn(l->listener_data);
}

struct event_table {
    array_list listeners;
};

static event_listener *event_resolve(const event_table *table, const event_handle handle) {
    if (!table || handle == EVENT_HANDLE_INVALID) return NULL;
    return (event_listener *)array_list_get(&table->listeners, (size_t)handle - 1u);
}

void event_table_init(event_table *event_table) {
    if (!event_table) return;
    array_list_init(&event_table->listeners, EVENT_DEFAULT_LISTENER_CAPACITY,
                    sizeof(event_listener));
}

void event_table_destroy(event_table *event_table) {
    if (!event_table) return;

    for (size_t i = 0; i < event_table->listeners.header.size; i++) {
        event_listener *l = (event_listener *)array_list_get(&event_table->listeners, i);
        if (l) event_listener_release(l);
    }
    array_list_deinit(&event_table->listeners);
}

event_handle event_table_subscribe(event_table *event_table,
                                   const event_category category, const int event_id,
                                   const event_callback callback, void *userdata,
                                   const event_userdata_destroy destroy_fn) {
    if (!event_table || !callback) return EVENT_HANDLE_INVALID;

    event_identifier identifier = {
         .category = category,
         .event_id = event_id  //
    };

    event_listener listener = {
         .identifier    = identifier,
         .callback      = callback,
         .listener_data = userdata,
         .destroy_fn    = destroy_fn  //
    };

    const size_t index = event_table->listeners.header.size;
    if (!array_list_append(&event_table->listeners, &listener)) {
#if EVENT_LOG_ENABLED
        log_debug("event.subscribe: append failed category=%d event_id=%d", category,
                  event_id);
#endif
        return EVENT_HANDLE_INVALID;
    }

#if EVENT_LOG_ENABLED
    log_debug("event.subscribe: category=%d event_id=%d userdata=%p handle=%u", category,
              event_id, userdata, (unsigned int)(index + 1u));
#endif
    return (event_handle)(index + 1u);
}

bool event_table_unsubscribe(event_table *event_table, const event_handle handle) {
    event_listener *l = event_resolve(event_table, handle);
    if (!l) return false;

#if EVENT_LOG_ENABLED
    log_debug("event.unsubscribe: handle=%u category=%d event_id=%d", handle,
              l->identifier.category, l->identifier.event_id);
#endif
    event_listener_release(l);
    return array_list_remove(&event_table->listeners, (size_t)handle - 1u);
}

bool event_table_unsubscribe_by_event_identifier(event_table         *event_table,
                                                 const event_category category,
                                                 const int            event_id) {
    if (!event_table) return false;

    bool removed = false;
    for (size_t i = event_table->listeners.header.size; i-- > 0;) {
        event_listener *l = (event_listener *)array_list_get(&event_table->listeners, i);
        if (!l) continue;
        if (l->identifier.category == category && l->identifier.event_id == event_id) {
            event_listener_release(l);
            array_list_remove(&event_table->listeners, i);
            removed = true;
        }
    }
#if EVENT_LOG_ENABLED
    log_debug("event.unsubscribe_by_event_identifier: category=%d event_id=%d removed=%d",
              category, event_id, removed);
#endif
    return removed;
}

void event_table_publish(event_table *event_table, event_context *ctx) {
    if (!event_table) return;

    for (size_t i = 0; i < event_table->listeners.header.size; i++) {
        event_listener *l = array_list_get(&event_table->listeners, i);

        if (l->identifier.category == ctx->identifier.category &&
            l->identifier.event_id == ctx->identifier.event_id) {
#if EVENT_LOG_ENABLED
            log_trace("event.publish: category=%d event_id=%d userdata=%p",
                      ctx->identifier.category, ctx->identifier.event_id,
                      l->listener_data);
#endif
            // @todo: what to do with the result
            l->callback(ctx, l->listener_data);
        }
    }
}
#endif

#endif
