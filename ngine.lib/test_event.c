#include "../thirdparty/minunit.h"

#define ARRAY_LIST_IMPLEMENTATION
#define EVENT_IMPLEMENTATION
#include "event.h"

typedef struct {
    int            call_count;
    event_category last_category;
    int            last_event_id;
    void          *last_payload;
    size_t         last_payload_size;
    void          *last_userdata;
    int            order;
    int           *order_counter;
    bool           destroyed;
} spy_record;

static event_callback_result spy_callback(event_context *ctx, void *userdata) {
    spy_record *rec = (spy_record *)userdata;
    rec->call_count++;
    rec->last_category     = ctx->identifier.category;
    rec->last_event_id     = ctx->identifier.event_id;
    rec->last_payload      = ctx->payload;
    rec->last_payload_size = ctx->payload_size;
    rec->last_userdata     = userdata;
    if (rec->order_counter) rec->order = ++(*rec->order_counter);
    return (event_callback_result){.type = EVENT_CALLBACK_RESULT_VOID};
}

static event_callback_result spy_callback_b(event_context *ctx, void *userdata) {
    return spy_callback(ctx, userdata);
}

static void spy_destroy(void *userdata) { ((spy_record *)userdata)->destroyed = true; }

static spy_record spy_record_new(void) {
    return (spy_record){
         .call_count        = 0,
         .last_category     = 0,
         .last_event_id     = 0,
         .last_payload      = NULL,
         .last_payload_size = 0,
         .last_userdata     = NULL,
         .order             = 0,
         .order_counter     = NULL,
         .destroyed         = false  //
    };
}

MU_TEST(test_init_creates_empty_table) {
    event_table table;
    event_table_init(&table);
    mu_assert_int_eq(0, (int)table.listeners.header.size);
    event_table_destroy(&table);
}

MU_TEST(test_init_null_table_safe) { event_table_init(NULL); }

MU_TEST(test_destroy_null_table_safe) { event_table_destroy(NULL); }

MU_TEST(test_subscribe_adds_listener) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    mu_check(event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec,
                                   NULL));
    mu_assert_int_eq(1, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_subscribe_multiple_listeners_same_event) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    mu_check(event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback,
                                   &rec_a, NULL));
    mu_check(event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback,
                                   &rec_b, NULL));
    mu_assert_int_eq(2, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_subscribe_null_table_returns_false) {
    spy_record rec = spy_record_new();
    mu_check(!event_table_subscribe(NULL, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec,
                                    NULL));
}

MU_TEST(test_subscribe_null_callback_returns_false) {
    event_table table;
    event_table_init(&table);

    mu_check(!event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, NULL, NULL, NULL));

    event_table_destroy(&table);
}

MU_TEST(test_subscribe_duplicate_allowed) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    mu_check(event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec,
                                   NULL));
    mu_check(event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec,
                                   NULL));
    mu_assert_int_eq(2, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_removes_exact_match) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    mu_check(
         event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec));
    mu_assert_int_eq(0, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_no_match_returns_false) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    mu_check(
         !event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 2, spy_callback, &rec));
    mu_assert_int_eq(1, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_empty_table_returns_false) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    mu_check(
         !event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec));

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_null_table_returns_false) {
    spy_record rec = spy_record_new();
    mu_check(
         !event_table_unsubscribe(NULL, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec));
}

MU_TEST(test_unsubscribe_null_callback_returns_false) {
    event_table table;
    event_table_init(&table);

    mu_check(!event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, NULL, NULL));

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_one_leaves_others_intact) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_a, NULL);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_b, NULL);

    mu_check(event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback,
                                     &rec_a));
    mu_assert_int_eq(1, (int)table.listeners.header.size);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 1},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(0, rec_a.call_count);
    mu_assert_int_eq(1, rec_b.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_matches_only_target_callback) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_a, NULL);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback_b, &rec_b,
                          NULL);

    mu_check(event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback,
                                     &rec_a));
    mu_assert_int_eq(1, (int)table.listeners.header.size);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 1},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(0, rec_a.call_count);
    mu_assert_int_eq(1, rec_b.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_by_id_removes_all_matching) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_a, NULL);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback_b, &rec_b,
                          NULL);

    mu_check(
         event_table_unsubscribe_by_event_identifier(&table, EVENT_TAG_SYSTEM_INPUT, 1));
    mu_assert_int_eq(0, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_by_id_leaves_other_ids_intact) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_a, NULL);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 2, spy_callback, &rec_b, NULL);

    mu_check(
         event_table_unsubscribe_by_event_identifier(&table, EVENT_TAG_SYSTEM_INPUT, 1));
    mu_assert_int_eq(1, (int)table.listeners.header.size);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 2},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(0, rec_a.call_count);
    mu_assert_int_eq(1, rec_b.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_by_id_no_match_returns_false) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    mu_check(
         !event_table_unsubscribe_by_event_identifier(&table, EVENT_TAG_SYSTEM_INPUT, 2));
    mu_assert_int_eq(1, (int)table.listeners.header.size);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_by_id_empty_table_returns_false) {
    event_table table;
    event_table_init(&table);

    mu_check(
         !event_table_unsubscribe_by_event_identifier(&table, EVENT_TAG_SYSTEM_INPUT, 1));

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_by_id_null_table_returns_false) {
    mu_check(
         !event_table_unsubscribe_by_event_identifier(NULL, EVENT_TAG_SYSTEM_INPUT, 1));
}

MU_TEST(test_unsubscribe_calls_destroy_fn) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec,
                          spy_destroy);

    mu_check(
         event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec));
    mu_check(rec.destroyed);

    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_no_destroy_fn_leaves_userdata_alone) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    mu_check(
         event_table_unsubscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec));
    mu_check(!rec.destroyed);
    event_table_destroy(&table);
}

MU_TEST(test_unsubscribe_by_id_calls_destroy_fn_for_each) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_a,
                          spy_destroy);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback_b, &rec_b,
                          spy_destroy);

    mu_check(
         event_table_unsubscribe_by_event_identifier(&table, EVENT_TAG_SYSTEM_INPUT, 1));
    mu_check(rec_a.destroyed);
    mu_check(rec_b.destroyed);

    event_table_destroy(&table);
}

MU_TEST(test_destroy_table_calls_destroy_fn_for_remaining_listeners) {
    event_table table;
    event_table_init(&table);

    spy_record rec_a = spy_record_new();
    spy_record rec_b = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec_a,
                          spy_destroy);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 2, spy_callback, &rec_b,
                          spy_destroy);

    event_table_destroy(&table);

    mu_check(rec_a.destroyed);
    mu_check(rec_b.destroyed);
}

MU_TEST(test_destroy_table_skips_null_destroy_fn) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    event_table_destroy(&table);

    mu_check(!rec.destroyed);
}

MU_TEST(test_publish_does_not_call_destroy_fn) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec,
                          spy_destroy);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 1},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_check(!rec.destroyed);
    mu_assert_int_eq(1, rec.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_publish_calls_matching_listener) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    int           payload = 42;
    event_context ctx     = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 1},
         .payload      = &payload,
         .payload_size = sizeof(payload)};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_category == EVENT_TAG_SYSTEM_INPUT);
    mu_assert_int_eq(1, rec.last_event_id);
    mu_check(rec.last_payload == &payload);
    mu_assert_int_eq((int)sizeof(payload), (int)rec.last_payload_size);
    mu_check(rec.last_userdata == &rec);

    event_table_destroy(&table);
}

MU_TEST(test_publish_ignores_different_event_id) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 2},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(0, rec.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_publish_ignores_different_category) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_INPUT, 1, spy_callback, &rec, NULL);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_AUDIO, .event_id = 1},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(0, rec.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_publish_null_table_safe) {
    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_INPUT, .event_id = 1},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(NULL, &ctx);
}

MU_TEST(test_integration_subscribe_publish_fires) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, &rec, NULL);

    event_context ctx = {.identifier = {.category = EVENT_TAG_APPLICATION, .event_id = 7},
                         .payload    = NULL,
                         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(1, rec.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_integration_two_listeners_fire_in_order) {
    event_table table;
    event_table_init(&table);

    int        order_counter = 0;
    spy_record rec_a         = spy_record_new();
    spy_record rec_b         = spy_record_new();
    rec_a.order_counter      = &order_counter;
    rec_b.order_counter      = &order_counter;

    event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, &rec_a, NULL);
    event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, &rec_b, NULL);

    event_context ctx = {.identifier = {.category = EVENT_TAG_APPLICATION, .event_id = 7},
                         .payload    = NULL,
                         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(1, rec_a.call_count);
    mu_assert_int_eq(1, rec_b.call_count);
    mu_assert_int_eq(1, rec_a.order);
    mu_assert_int_eq(2, rec_b.order);

    event_table_destroy(&table);
}

MU_TEST(test_integration_unsubscribe_then_publish_does_not_fire) {
    event_table table;
    event_table_init(&table);

    spy_record rec = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, &rec, NULL);
    event_table_unsubscribe(&table, EVENT_TAG_APPLICATION, 7, spy_callback, &rec);

    event_context ctx = {.identifier = {.category = EVENT_TAG_APPLICATION, .event_id = 7},
                         .payload    = NULL,
                         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(0, rec.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_integration_full_flow) {
    event_table table;
    event_table_init(&table);
    mu_assert_int_eq(0, (int)table.listeners.header.size);

    spy_record rec = spy_record_new();
    mu_check(event_table_subscribe(&table, EVENT_TAG_APPLICATION, 9, spy_callback, &rec,
                                   NULL));

    int           payload = 5;
    event_context ctx = {.identifier = {.category = EVENT_TAG_APPLICATION, .event_id = 9},
                         .payload    = &payload,
                         .payload_size = (size_t)sizeof(payload)};
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(1, rec.call_count);
    mu_check(rec.last_payload == &payload);

    mu_check(
         event_table_unsubscribe(&table, EVENT_TAG_APPLICATION, 9, spy_callback, &rec));
    event_table_publish(&table, &ctx);
    mu_assert_int_eq(1, rec.call_count);

    event_table_destroy(&table);
}

MU_TEST(test_integration_different_categories_do_not_cross_trigger) {
    event_table table;
    event_table_init(&table);

    spy_record rec_window = spy_record_new();
    spy_record rec_audio  = spy_record_new();
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_WINDOW, 3, spy_callback, &rec_window,
                          NULL);
    event_table_subscribe(&table, EVENT_TAG_SYSTEM_AUDIO, 3, spy_callback, &rec_audio,
                          NULL);

    event_context ctx = {
         .identifier   = {.category = EVENT_TAG_SYSTEM_WINDOW, .event_id = 3},
         .payload      = NULL,
         .payload_size = 0};
    event_table_publish(&table, &ctx);

    mu_assert_int_eq(1, rec_window.call_count);
    mu_assert_int_eq(0, rec_audio.call_count);

    event_table_destroy(&table);
}

MU_TEST_SUITE(event_suite) {
    MU_RUN_TEST(test_init_creates_empty_table);
    MU_RUN_TEST(test_init_null_table_safe);
    MU_RUN_TEST(test_destroy_null_table_safe);

    MU_RUN_TEST(test_subscribe_adds_listener);
    MU_RUN_TEST(test_subscribe_multiple_listeners_same_event);
    MU_RUN_TEST(test_subscribe_null_table_returns_false);
    MU_RUN_TEST(test_subscribe_null_callback_returns_false);
    MU_RUN_TEST(test_subscribe_duplicate_allowed);

    MU_RUN_TEST(test_unsubscribe_removes_exact_match);
    MU_RUN_TEST(test_unsubscribe_no_match_returns_false);
    MU_RUN_TEST(test_unsubscribe_empty_table_returns_false);
    MU_RUN_TEST(test_unsubscribe_null_table_returns_false);
    MU_RUN_TEST(test_unsubscribe_null_callback_returns_false);
    MU_RUN_TEST(test_unsubscribe_one_leaves_others_intact);
    MU_RUN_TEST(test_unsubscribe_matches_only_target_callback);

    MU_RUN_TEST(test_unsubscribe_by_id_removes_all_matching);
    MU_RUN_TEST(test_unsubscribe_by_id_leaves_other_ids_intact);
    MU_RUN_TEST(test_unsubscribe_by_id_no_match_returns_false);
    MU_RUN_TEST(test_unsubscribe_by_id_empty_table_returns_false);
    MU_RUN_TEST(test_unsubscribe_by_id_null_table_returns_false);

    MU_RUN_TEST(test_unsubscribe_calls_destroy_fn);
    MU_RUN_TEST(test_unsubscribe_no_destroy_fn_leaves_userdata_alone);
    MU_RUN_TEST(test_unsubscribe_by_id_calls_destroy_fn_for_each);
    MU_RUN_TEST(test_destroy_table_calls_destroy_fn_for_remaining_listeners);
    MU_RUN_TEST(test_destroy_table_skips_null_destroy_fn);
    MU_RUN_TEST(test_publish_does_not_call_destroy_fn);

    MU_RUN_TEST(test_publish_calls_matching_listener);
    MU_RUN_TEST(test_publish_ignores_different_event_id);
    MU_RUN_TEST(test_publish_ignores_different_category);
    MU_RUN_TEST(test_publish_null_table_safe);

    MU_RUN_TEST(test_integration_subscribe_publish_fires);
    MU_RUN_TEST(test_integration_two_listeners_fire_in_order);
    MU_RUN_TEST(test_integration_unsubscribe_then_publish_does_not_fire);
    MU_RUN_TEST(test_integration_different_categories_do_not_cross_trigger);
    MU_RUN_TEST(test_integration_full_flow);
}

int main(void) {
    MU_RUN_SUITE(event_suite);
    MU_REPORT();
    return MU_EXIT_CODE;
}
