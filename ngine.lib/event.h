#ifndef EVENT_H
#define EVENT_H

typedef struct event_table           event_table;
typedef struct event_context         event_context;
typedef struct event_callback_result event_callback_result;
typedef event_callback_result (*event_callback)(event_context *ctx, void *userdata);

typedef enum {
    EVENT_TAG_SYSTEM_WINDOW,
    EVENT_TAG_SYSTEM_RENDERING,
    EVENT_TAG_SYSTEM_INPUT,
    EVENT_TAG_SYSTEM_AUDIO,
    EVENT_TAG_APPLICATION,
    EVENT_TAG_CUSTOM
} event_category;

typedef enum {
    EVENT_CALLBACK_RESULT_VOID,
    EVENT_CALLBACK_RESULT_INT,
    EVENT_CALLBACK_RESULT_FLOAT,
    EVENT_CALLBACK_RESULT_STRING,
    EVENT_CALLBACK_RESULT_PTR,
    EVENT_CALLBACK_RESULT_ERROR,
} event_callback_result_type;

void event_table_init(event_table *event_table);
void event_table_destroy(event_table *event_table);
bool event_table_subscribe(event_table *event_table, const event_category category,
                           const int event_id, const event_callback callback,
                           void *userdata);
bool event_table_unsubscribe(event_table *event_table, const event_category category,
                             const int event_id, const event_callback callback,
                             void *userdata);
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
    const event_category category;
    const int            event_id;
} event_identifier;

typedef struct {
    const event_identifier identifier;
    const event_callback   callback;
    void                  *listener_data;
} event_listener;

struct event_table {
    array_list listeners;
};

struct event_context {
    const event_identifier identifier;

    void  *payload;
    size_t payload_size;
};

struct event_callback_result {
    const event_callback_result_type type;
    union {
        int         i;
        float       f;
        const char *str;
        void       *ptr;
    } value;
};

void event_table_init(event_table *event_table) {
    if (!event_table) return;
    array_list_init(&event_table->listeners, EVENT_DEFAULT_LISTENER_CAPACITY,
                    sizeof(event_listener));
}

void event_table_destroy(event_table *event_table) {
    if (!event_table) return;
    array_list_deinit(&event_table->listeners);
}

bool event_table_subscribe(event_table *event_table, const event_category category,
                           const int event_id, const event_callback callback,
                           void *userdata) {
    if (!event_table || !callback) return false;

    event_identifier identifier = {
         .category = category,
         .event_id = event_id  //
    };

    event_listener listener = {
         .identifier    = identifier,
         .callback      = callback,
         .listener_data = userdata  //
    };

    return array_list_append(&event_table->listeners, &listener);
}

bool event_table_unsubscribe(event_table *event_table, const event_category category,
                             const int event_id, const event_callback callback,
                             void *userdata) {
    if (!event_table || !callback) return false;

    event_identifier identifier = {
         .category = category,
         .event_id = event_id  //
    };

    event_listener input_listener = {
         .identifier    = identifier,
         .callback      = callback,
         .listener_data = userdata  //
    };
    for (size_t i = 0; i < event_table->listeners.header.size; i++) {
        event_listener *l = (event_listener *)array_list_get(&event_table->listeners, i);
        if (!l) continue;
        if (l->identifier.category == input_listener.identifier.category &&
            l->identifier.event_id == input_listener.identifier.event_id &&
            l->callback == input_listener.callback &&
            l->listener_data == input_listener.listener_data) {
            array_list_remove(&event_table->listeners, i);
            return true;
        }
    }
    return false;
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
            array_list_remove(&event_table->listeners, i);
            removed = true;
        }
    }
    return removed;
}

void event_table_publish(event_table *event_table, event_context *ctx) {
    if (!event_table) return;

    for (size_t i = 0; i < event_table->listeners.header.size; i++) {
        event_listener *l = array_list_get(&event_table->listeners, i);

        if (l->identifier.category == ctx->identifier.category &&
            l->identifier.event_id == ctx->identifier.event_id) {
            // @todo: what to do with the result
            l->callback(ctx, l->listener_data);
        }
    }
}
#endif

#endif
